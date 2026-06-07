#ifndef SKAILAR_HTTP_CLIENT_HPP
#define SKAILAR_HTTP_CLIENT_HPP

#include <skailar/config.hpp>
#include <skailar/errors.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>

#include <curl/curl.h>

namespace skailar::detail {

/// Records whether a request may be safely replayed after a 5xx or transport
/// failure.
enum class Idempotency : std::uint8_t {
    /// GET requests: retried on 5xx and transient transport errors.
    Idempotent,
    /// Billable POSTs: retried only on 429, never on 5xx, to avoid double
    /// charging.
    SideEffect,
};

/// Resolved, immutable client state shared by every request. Owns the libcurl
/// share handle that lets concurrent easy handles pool connections safely.
class HttpContext {
public:
    explicit HttpContext(ClientConfig config);
    ~HttpContext();

    HttpContext(const HttpContext&) = delete;
    HttpContext& operator=(const HttpContext&) = delete;
    HttpContext(HttpContext&&) = delete;
    HttpContext& operator=(HttpContext&&) = delete;

    const std::string& api_key() const noexcept { return api_key_; }
    const std::string& base_url() const noexcept { return base_url_; }
    const std::map<std::string, std::string>& default_headers() const noexcept {
        return default_headers_;
    }
    std::chrono::milliseconds timeout() const noexcept { return timeout_; }
    int max_retries() const noexcept { return max_retries_; }
    CURLSH* share() const noexcept { return share_; }

    /// Joins the base URL with a request path, avoiding a double slash.
    std::string endpoint(const std::string& path) const;

private:
    std::string api_key_;
    std::string base_url_;
    std::map<std::string, std::string> default_headers_;
    std::chrono::milliseconds timeout_;
    int max_retries_;

    CURLSH* share_ = nullptr;
    std::array<std::mutex, 4> share_mutexes_;
};

/// The outcome of a buffered request: a decoded success body, or an Error.
struct Response {
    long status = 0;
    std::string body;
    std::string request_id;
    int retry_after = 0;
};

/// Configures a curl easy handle with the shared connection pool, default and
/// per-call headers (deduplicated case-insensitively), the JSON or supplied
/// Accept header, and the bearer token applied last. The returned curl_slist is
/// owned by the caller and must be freed with curl_slist_free_all after the
/// transfer.
curl_slist* build_headers(const HttpContext& ctx,
                          const std::map<std::string, std::string>& per_call, bool has_json_body,
                          const std::string& accept);

/// Maps a libcurl easy result code to an Error, distinguishing timeout from
/// other network failures.
Error transport_error(CURLcode code, const char* detail);

/// Builds an Error from a non-2xx response body, choosing the kind from status.
Error api_error(long status, const std::string& request_id, int retry_after, std::string body);

/// Parses a Retry-After header value (integer seconds), returning 0 when absent
/// or malformed.
int parse_retry_after(const std::string& value);

/// Reports whether a non-2xx status warrants another attempt given idempotency.
bool should_retry(long status, Idempotency idem, int attempt, int max_attempts);

/// Sleeps before the next retry: a Retry-After value (capped at 60s) takes
/// precedence, otherwise exponential backoff with full jitter, base 500ms,
/// capped at 8s.
void sleep_backoff(int attempt, int retry_after_secs);

/// Executes a buffered request with retries, returning the success body. POST
/// bodies are JSON; @p body is empty for GET. @throws skailar::Error.
Response do_request(const HttpContext& ctx, const char* method, const std::string& path,
                    const std::string& body, Idempotency idem);

} // namespace skailar::detail

#endif // SKAILAR_HTTP_CLIENT_HPP
