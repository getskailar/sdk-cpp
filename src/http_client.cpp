#include "http_client.hpp"

#include "internal/string_utils.hpp"

#include <skailar/errors.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <exception>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace skailar::detail {

namespace {

constexpr long kHttpOk = 200;
constexpr long kHttpMultipleChoices = 300;
constexpr long kHttpBadRequest = 400;
constexpr long kHttpUnauthorized = 401;
constexpr long kHttpNotFound = 404;
constexpr long kHttpTooManyRequests = 429;
constexpr long kHttpServerError = 500;

constexpr auto kBackoffBase = std::chrono::milliseconds(500);
constexpr auto kBackoffCap = std::chrono::milliseconds(8000);
constexpr int kMaxRetryAfterSecs = 60;
constexpr int kBackoffMaxShift = 20;
constexpr std::size_t kMaxResponseBytes = std::size_t {32} * 1024 * 1024;

ErrorKind kind_for_status(long status) noexcept {
    if (status == kHttpUnauthorized) {
        return ErrorKind::Auth;
    }
    if (status == kHttpBadRequest) {
        return ErrorKind::BadRequest;
    }
    if (status == kHttpNotFound) {
        return ErrorKind::NotFound;
    }
    if (status == kHttpTooManyRequests) {
        return ErrorKind::RateLimit;
    }
    if (status >= kHttpServerError) {
        return ErrorKind::Upstream;
    }
    return ErrorKind::Api;
}

// Parses (code, message) from a Skailar or OpenAI error body, tolerating a
// nested object, a flat string, or a top-level message.
std::pair<std::string, std::string> parse_error_fields(const std::string& body) {
    if (body.empty()) {
        return {};
    }
    nlohmann::json parsed = nlohmann::json::parse(body, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        return {};
    }

    std::string top_message;
    if (auto it = parsed.find("message"); it != parsed.end() && it->is_string()) {
        top_message = it->get<std::string>();
    }

    auto err = parsed.find("error");
    if (err == parsed.end()) {
        return {std::string(), top_message};
    }
    if (err->is_string()) {
        return {err->get<std::string>(), top_message};
    }
    if (err->is_object()) {
        std::string code;
        if (auto it = err->find("type"); it != err->end() && it->is_string()) {
            code = it->get<std::string>();
        }
        if (code.empty()) {
            if (auto it = err->find("code"); it != err->end() && it->is_string()) {
                code = it->get<std::string>();
            }
        }
        std::string message = top_message;
        if (auto it = err->find("message"); it != err->end() && it->is_string()) {
            message = it->get<std::string>();
        }
        return {code, message};
    }
    return {std::string(), top_message};
}

std::size_t append_body(char* data, std::size_t size, std::size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    const std::size_t total = size * nmemb;
    if (out->size() + total > kMaxResponseBytes) {
        return 0; // Abort the transfer; surfaces as a write error.
    }
    out->append(data, total);
    return total;
}

std::size_t collect_header(char* buffer, std::size_t size, std::size_t nitems, void* userdata) {
    auto* headers = static_cast<std::map<std::string, std::string>*>(userdata);
    const std::size_t total = size * nitems;
    std::string_view line(buffer, total);
    const auto colon = line.find(':');
    if (colon != std::string_view::npos) {
        std::string_view name = trim(line.substr(0, colon));
        std::string_view value = trim(line.substr(colon + 1));
        std::string key;
        key.reserve(name.size());
        for (char c : name) {
            key.push_back(ascii_lower(c));
        }
        (*headers)[key] = std::string(value);
    }
    return total;
}

std::string header_value(const std::map<std::string, std::string>& headers,
                         const char* lower_name) {
    auto it = headers.find(lower_name);
    return it == headers.end() ? std::string() : it->second;
}

std::string extract_request_id(const std::map<std::string, std::string>& headers) {
    for (const char* name : {"x-request-id", "x-skailar-request-id", "request-id"}) {
        if (auto v = header_value(headers, name); !v.empty()) {
            return v;
        }
    }
    return {};
}

void share_lock(CURL*, curl_lock_data data, curl_lock_access, void* userptr) {
    auto* mutexes = static_cast<std::array<std::mutex, 4>*>(userptr);
    const std::size_t index = static_cast<std::size_t>(data) % mutexes->size();
    mutexes->at(index).lock();
}

void share_unlock(CURL*, curl_lock_data data, void* userptr) {
    auto* mutexes = static_cast<std::array<std::mutex, 4>*>(userptr);
    const std::size_t index = static_cast<std::size_t>(data) % mutexes->size();
    mutexes->at(index).unlock();
}

struct CurlGlobalInit {
    CurlGlobalInit() { curl_global_init(CURL_GLOBAL_DEFAULT); }
};

void ensure_curl_global() {
    static CurlGlobalInit init;
    (void)init;
}

} // namespace

HttpContext::HttpContext(ClientConfig config)
    : api_key_(std::move(config.api_key)), base_url_(std::move(config.base_url)),
      default_headers_(std::move(config.default_headers)), timeout_(config.timeout),
      max_retries_(std::max(0, config.max_retries)) {
    ensure_curl_global();
    share_ = curl_share_init();
    if (share_ != nullptr) {
        curl_share_setopt(share_, CURLSHOPT_LOCKFUNC, share_lock);
        curl_share_setopt(share_, CURLSHOPT_UNLOCKFUNC, share_unlock);
        curl_share_setopt(share_, CURLSHOPT_USERDATA, &share_mutexes_);
        curl_share_setopt(share_, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
        curl_share_setopt(share_, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
        curl_share_setopt(share_, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
    }
}

HttpContext::~HttpContext() {
    if (share_ != nullptr) {
        curl_share_cleanup(share_);
    }
}

std::string HttpContext::endpoint(const std::string& path) const {
    std::size_t start = 0;
    while (start < path.size() && path[start] == '/') {
        ++start;
    }
    return base_url_ + "/" + path.substr(start);
}

curl_slist* build_headers(const HttpContext& ctx,
                          const std::map<std::string, std::string>& per_call, bool has_json_body,
                          const std::string& accept) {
    // Merge default and per-call headers with a case-insensitive key so a later
    // header replaces an earlier one regardless of letter case.
    std::vector<std::pair<std::string, std::string>> merged;
    auto upsert = [&merged](const std::string& name, const std::string& value) {
        // The Authorization header is owned by the SDK; never let a caller set it.
        if (iequals(name, "authorization")) {
            return;
        }
        for (auto& entry : merged) {
            if (iequals(entry.first, name)) {
                entry.second = value;
                return;
            }
        }
        merged.emplace_back(name, value);
    };

    for (const auto& [name, value] : ctx.default_headers()) {
        upsert(name, value);
    }
    for (const auto& [name, value] : per_call) {
        upsert(name, value);
    }

    bool has_accept = false;
    bool has_content_type = false;
    for (const auto& [name, value] : merged) {
        if (iequals(name, "accept")) {
            has_accept = true;
        }
        if (iequals(name, "content-type")) {
            has_content_type = true;
        }
    }
    if (!has_accept) {
        merged.emplace_back("Accept", accept.empty() ? "application/json" : accept);
    }
    if (has_json_body && !has_content_type) {
        merged.emplace_back("Content-Type", "application/json");
    }

    curl_slist* list = nullptr;
    for (const auto& [name, value] : merged) {
        const std::string header = name + ": " + value;
        list = curl_slist_append(list, header.c_str());
    }
    const std::string auth = "Authorization: Bearer " + ctx.api_key();
    list = curl_slist_append(list, auth.c_str());
    return list;
}

Error transport_error(CURLcode code, const char* detail) {
    const std::string message = (detail != nullptr && detail[0] != '\0')
        ? std::string(detail)
        : std::string(curl_easy_strerror(code));
    if (code == CURLE_OPERATION_TIMEDOUT) {
        return Error(ErrorKind::Timeout, 0, "request timed out");
    }
    return Error(ErrorKind::Network, 0, "network error: " + message);
}

Error api_error(long status, const std::string& request_id, int retry_after, std::string body) {
    auto [code, message] = parse_error_fields(body);
    if (message.empty()) {
        const std::string trimmed(trim(body));
        message = trimmed.empty() ? ("HTTP " + std::to_string(status)) : trimmed;
    }
    return Error(kind_for_status(status), static_cast<int>(status), std::move(message),
                 std::move(code), request_id, std::move(body), retry_after);
}

int parse_retry_after(const std::string& value) {
    const std::string trimmed(trim(value));
    if (trimmed.empty()) {
        return 0;
    }
    try {
        std::size_t consumed = 0;
        const int secs = std::stoi(trimmed, &consumed);
        if (consumed != trimmed.size() || secs < 0) {
            return 0;
        }
        return secs;
    } catch (const std::exception&) {
        return 0;
    }
}

bool should_retry(long status, Idempotency idem, int attempt, int max_attempts) {
    if (attempt + 1 >= max_attempts) {
        return false;
    }
    if (status == kHttpTooManyRequests) {
        return true;
    }
    return status >= kHttpServerError && idem == Idempotency::Idempotent;
}

void sleep_backoff(int attempt, int retry_after_secs) {
    std::chrono::milliseconds delay {0};
    if (retry_after_secs > 0) {
        const int capped = std::min(retry_after_secs, kMaxRetryAfterSecs);
        delay = std::chrono::seconds(capped);
    } else {
        const int shift = std::min(attempt, kBackoffMaxShift);
        std::int64_t window = kBackoffBase.count() << shift;
        window = std::min<std::int64_t>(window, kBackoffCap.count());
        if (window <= 0) {
            return;
        }
        thread_local std::mt19937_64 rng {std::random_device {}()};
        std::uniform_int_distribution<std::int64_t> dist(0, window);
        delay = std::chrono::milliseconds(dist(rng));
    }
    if (delay.count() > 0) {
        std::this_thread::sleep_for(delay);
    }
}

namespace {

struct Attempt {
    CURLcode code = CURLE_OK;
    long status = 0;
    std::string body;
    std::string request_id;
    int retry_after = 0;
    std::array<char, CURL_ERROR_SIZE> error_buffer {};
};

Attempt perform(const HttpContext& ctx, const std::string& url, const std::string& body,
                bool has_body) {
    Attempt result;
    CURL* easy = curl_easy_init();
    if (easy == nullptr) {
        result.code = CURLE_FAILED_INIT;
        return result;
    }

    std::map<std::string, std::string> response_headers;
    curl_slist* headers = build_headers(ctx, {}, has_body, "application/json");

    curl_easy_setopt(easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(easy, CURLOPT_SHARE, ctx.share());
    curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, append_body);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, &result.body);
    curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, collect_header);
    curl_easy_setopt(easy, CURLOPT_HEADERDATA, &response_headers);
    curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, result.error_buffer.data());
    curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(easy, CURLOPT_ACCEPT_ENCODING, "");

    const auto timeout = ctx.timeout();
    if (timeout.count() > 0) {
        curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, static_cast<long>(timeout.count()));
    }

    if (has_body) {
        curl_easy_setopt(easy, CURLOPT_POST, 1L);
        curl_easy_setopt(easy, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    } else {
        curl_easy_setopt(easy, CURLOPT_HTTPGET, 1L);
    }

    result.code = curl_easy_perform(easy);
    if (result.code == CURLE_OK) {
        curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &result.status);
        result.request_id = extract_request_id(response_headers);
        result.retry_after = parse_retry_after(header_value(response_headers, "retry-after"));
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(easy);
    return result;
}

bool is_retryable_transport(CURLcode code) noexcept {
    switch (code) {
    case CURLE_OPERATION_TIMEDOUT:
    case CURLE_COULDNT_CONNECT:
    case CURLE_COULDNT_RESOLVE_HOST:
    case CURLE_COULDNT_RESOLVE_PROXY:
    case CURLE_SEND_ERROR:
    case CURLE_RECV_ERROR:
    case CURLE_GOT_NOTHING:
    case CURLE_PARTIAL_FILE:
        return true;
    default:
        return false;
    }
}

} // namespace

Response do_request(const HttpContext& ctx, const char* method, const std::string& path,
                    const std::string& body, Idempotency idem) {
    const std::string url = ctx.endpoint(path);
    const int max_attempts = ctx.max_retries() + 1;
    const bool has_body = std::string_view(method) == "POST";

    for (int attempt = 0;; ++attempt) {
        Attempt res = perform(ctx, url, body, has_body);

        if (res.code != CURLE_OK) {
            if (is_retryable_transport(res.code) && idem == Idempotency::Idempotent
                && attempt + 1 < max_attempts) {
                sleep_backoff(attempt, 0);
                continue;
            }
            throw transport_error(res.code, res.error_buffer.data());
        }

        if (res.status >= kHttpOk && res.status < kHttpMultipleChoices) {
            Response out;
            out.status = res.status;
            out.body = std::move(res.body);
            out.request_id = std::move(res.request_id);
            out.retry_after = res.retry_after;
            return out;
        }

        if (should_retry(res.status, idem, attempt, max_attempts)) {
            sleep_backoff(attempt, res.retry_after);
            continue;
        }

        throw api_error(res.status, res.request_id, res.retry_after, std::move(res.body));
    }
}

} // namespace skailar::detail
