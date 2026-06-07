#include <skailar/streaming.hpp>

#include "http_client.hpp"
#include "sse_parser.hpp"
#include "streaming_internal.hpp"

#include <skailar/chat.hpp>
#include <skailar/errors.hpp>

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace skailar {

namespace {

constexpr long kHttpOk = 200;
constexpr long kHttpMultipleChoices = 300;
constexpr int kPollTimeoutMs = 1000;

// Decodes a data-line payload into a chunk, surfacing an in-band error frame as
// an Error rather than a chunk.
ChatCompletionChunk decode_chunk(const std::string& payload) {
    nlohmann::json parsed = nlohmann::json::parse(payload, nullptr, false);
    if (parsed.is_discarded()) {
        throw Error(ErrorKind::Decode, 0, "malformed streaming event", {}, {}, payload);
    }
    if (parsed.is_object() && parsed.contains("error")) {
        std::string code;
        std::string message = "streaming error";
        const auto& err = parsed.at("error");
        if (err.is_object()) {
            if (auto it = err.find("type"); it != err.end() && it->is_string()) {
                code = it->get<std::string>();
            }
            if (auto it = err.find("message"); it != err.end() && it->is_string()) {
                message = it->get<std::string>();
            }
        } else if (err.is_string()) {
            code = err.get<std::string>();
        }
        throw Error(ErrorKind::Upstream, 500, std::move(message), std::move(code), {}, payload);
    }
    try {
        return parsed.get<ChatCompletionChunk>();
    } catch (const nlohmann::json::exception&) {
        throw Error(ErrorKind::Decode, 0, "malformed streaming event", {}, {}, payload);
    }
}

} // namespace

class ChatCompletionStream::Impl {
public:
    Impl(CURLM* multi, CURL* easy, curl_slist* headers)
        : multi_(multi), easy_(easy), headers_(headers) { }

    ~Impl() { close(); }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;

    static std::size_t write(char* data, std::size_t size, std::size_t nmemb, void* userdata) {
        auto* self = static_cast<Impl*>(userdata);
        const std::size_t total = size * nmemb;
        if (!self->confirmed_) {
            self->handshake_body_.append(data, total);
        }
        self->parser_.feed(data, total);
        return total;
    }

    /// Marks the response status as 2xx so the write callback stops buffering
    /// the handshake body.
    void confirm() noexcept {
        confirmed_ = true;
        handshake_body_.clear();
        handshake_body_.shrink_to_fit();
    }

    /// The bytes buffered before the status was confirmed; used to build an
    /// error from a non-2xx streaming response.
    const std::string& handshake_body() const noexcept { return handshake_body_; }

    /// Drives the transfer until it completes, buffering whatever arrives. Used
    /// to collect a full error body before throwing.
    void drain_to_completion() noexcept {
        int running = 1;
        while (running != 0) {
            if (curl_multi_perform(multi_, &running) != CURLM_OK) {
                break;
            }
            if (running != 0) {
                int numfds = 0;
                if (curl_multi_poll(multi_, nullptr, 0, kPollTimeoutMs, &numfds) != CURLM_OK) {
                    break;
                }
            }
        }
    }

    std::optional<ChatCompletionChunk> next() {
        if (finished_) {
            return std::nullopt;
        }

        for (;;) {
            std::string payload;
            switch (parser_.next_event(payload)) {
            case detail::SseParser::Status::Event:
                return decode_chunk(payload);
            case detail::SseParser::Status::Done:
                finished_ = true;
                return std::nullopt;
            case detail::SseParser::Status::NeedMore:
                break;
            }

            if (transfer_done_) {
                parser_.mark_eof();
                std::string payload2;
                const auto status = parser_.next_event(payload2);
                if (status == detail::SseParser::Status::Event) {
                    return decode_chunk(payload2);
                }
                finished_ = true;
                return std::nullopt;
            }

            pump();
        }
    }

    void close() noexcept {
        if (easy_ != nullptr) {
            if (multi_ != nullptr) {
                curl_multi_remove_handle(multi_, easy_);
            }
            curl_easy_cleanup(easy_);
            easy_ = nullptr;
        }
        if (multi_ != nullptr) {
            curl_multi_cleanup(multi_);
            multi_ = nullptr;
        }
        if (headers_ != nullptr) {
            curl_slist_free_all(headers_);
            headers_ = nullptr;
        }
        finished_ = true;
    }

private:
    // Advances the transfer until more data arrives or it completes.
    void pump() {
        int running = 0;
        const CURLMcode mc = curl_multi_perform(multi_, &running);
        if (mc != CURLM_OK) {
            throw Error(ErrorKind::Network, 0,
                        std::string("stream transport error: ") + curl_multi_strerror(mc));
        }

        check_completion();
        if (running == 0) {
            transfer_done_ = true;
            return;
        }

        int numfds = 0;
        const CURLMcode pc = curl_multi_poll(multi_, nullptr, 0, kPollTimeoutMs, &numfds);
        if (pc != CURLM_OK) {
            throw Error(ErrorKind::Network, 0,
                        std::string("stream transport error: ") + curl_multi_strerror(pc));
        }
    }

    // Drains completion messages, mapping a failed transfer to an Error.
    void check_completion() {
        CURLMsg* msg = nullptr;
        int in_queue = 0;
        while ((msg = curl_multi_info_read(multi_, &in_queue)) != nullptr) {
            if (msg->msg == CURLMSG_DONE) {
                transfer_done_ = true;
                const CURLcode result = msg->data.result;
                if (result != CURLE_OK) {
                    throw detail::transport_error(result, curl_easy_strerror(result));
                }
            }
        }
    }

    CURLM* multi_;
    CURL* easy_;
    curl_slist* headers_;
    detail::SseParser parser_;
    std::string handshake_body_;
    bool confirmed_ = false;
    bool transfer_done_ = false;
    bool finished_ = false;
};

ChatCompletionStream::ChatCompletionStream(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) { }

ChatCompletionStream::ChatCompletionStream(ChatCompletionStream&&) noexcept = default;
ChatCompletionStream& ChatCompletionStream::operator=(ChatCompletionStream&&) noexcept = default;
ChatCompletionStream::~ChatCompletionStream() = default;

std::optional<ChatCompletionChunk> ChatCompletionStream::next() {
    return impl_->next();
}

void ChatCompletionStream::close() noexcept {
    if (impl_) {
        impl_->close();
    }
}

namespace detail {

std::unique_ptr<ChatCompletionStream>
open_chat_stream(const HttpContext& ctx, const std::string& path, const std::string& body) {
    const std::string url = ctx.endpoint(path);
    curl_slist* headers = build_headers(ctx, {}, true, "text/event-stream");

    CURL* easy = curl_easy_init();
    CURLM* multi = curl_multi_init();
    if (easy == nullptr || multi == nullptr) {
        if (easy != nullptr) {
            curl_easy_cleanup(easy);
        }
        if (multi != nullptr) {
            curl_multi_cleanup(multi);
        }
        curl_slist_free_all(headers);
        throw Error(ErrorKind::Network, 0, "failed to initialize curl handles");
    }

    auto impl = std::make_unique<ChatCompletionStream::Impl>(multi, easy, headers);

    curl_easy_setopt(easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(easy, CURLOPT_SHARE, ctx.share());
    curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(easy, CURLOPT_POST, 1L);
    // COPYPOSTFIELDS copies the body into the handle, so it need not outlive
    // this call; set the size first as the documentation requires.
    curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(easy, CURLOPT_COPYPOSTFIELDS, body.c_str());
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, &ChatCompletionStream::Impl::write);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, impl.get());
    curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 0L);

    const auto timeout = ctx.timeout();
    if (timeout.count() > 0) {
        // The timeout governs establishing the response, not the open stream;
        // apply it as a connect timeout only.
        curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(timeout.count()));
    }

    curl_multi_add_handle(multi, easy);

    // Drive the transfer until the status line is known or it ends early.
    int running = 0;
    for (;;) {
        const CURLMcode mc = curl_multi_perform(multi, &running);
        if (mc != CURLM_OK) {
            throw Error(ErrorKind::Network, 0,
                        std::string("stream transport error: ") + curl_multi_strerror(mc));
        }
        long status = 0;
        curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
        if (status != 0 || running == 0) {
            break;
        }
        int numfds = 0;
        if (curl_multi_poll(multi, nullptr, 0, kPollTimeoutMs, &numfds) != CURLM_OK) {
            throw Error(ErrorKind::Network, 0, "stream transport error while connecting");
        }
    }

    long status = 0;
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
    if (status != 0 && (status < kHttpOk || status >= kHttpMultipleChoices)) {
        // Collect the full error body, then surface it as an Error. The handle
        // cleanup is handled by impl's destructor when it goes out of scope.
        impl->drain_to_completion();
        throw api_error(status, {}, 0, impl->handshake_body());
    }

    impl->confirm();
    return std::make_unique<ChatCompletionStream>(std::move(impl));
}

} // namespace detail
} // namespace skailar
