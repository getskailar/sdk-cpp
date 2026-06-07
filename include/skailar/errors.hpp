#ifndef SKAILAR_ERRORS_HPP
#define SKAILAR_ERRORS_HPP

#include <cstdint>
#include <exception>
#include <string>

namespace skailar {

/// Classifies an @ref Error so callers can branch without matching on strings.
enum class ErrorKind : std::uint8_t {
    /// A non-2xx response that does not map to a more specific kind.
    Api,
    /// A 401 response: missing, invalid, or revoked API key.
    Auth,
    /// A 400 response: the request was malformed.
    BadRequest,
    /// A 404 response: the resource does not exist.
    NotFound,
    /// A 429 response: the rate limit was exceeded.
    RateLimit,
    /// A 5xx response: the upstream provider failed or timed out.
    Upstream,
    /// A transport failure (DNS, TLS, connection reset).
    Network,
    /// A request that exceeded the configured timeout.
    Timeout,
    /// A request cancelled by the caller. Reserved for a future cancellation API.
    Aborted,
    /// A successful response whose body could not be decoded.
    Decode,
    /// A client misconfiguration, such as a missing API key.
    Config,
};

/// Returns the lowercase, snake_case name of @p kind (for example "rate_limit").
const char* to_string(ErrorKind kind) noexcept;

/// The single exception type thrown by every fallible operation in the SDK.
///
/// Discriminate with @ref kind. The HTTP status, machine-readable code, server
/// request id, raw body, and uncapped Retry-After value are available through
/// the corresponding accessors when applicable.
class Error : public std::exception {
public:
    /**
     * @param kind        Classification of the failure.
     * @param status      HTTP status code, or 0 when not applicable.
     * @param message     Human-readable error message.
     * @param code        Machine-readable code from the response body, if any.
     * @param request_id  Server-assigned request identifier, if present.
     * @param raw         Raw response body, if one was read.
     * @param retry_after Uncapped Retry-After value in seconds, set on a 429.
     */
    Error(ErrorKind kind, int status, std::string message, std::string code = {},
          std::string request_id = {}, std::string raw = {}, int retry_after = 0);

    /// Classification of the failure.
    ErrorKind kind() const noexcept { return kind_; }

    /// HTTP status code, or 0 when not applicable.
    int status() const noexcept { return status_; }

    /// Machine-readable error code from the response body, or empty.
    const std::string& code() const noexcept { return code_; }

    /// Human-readable error message.
    const std::string& message() const noexcept { return message_; }

    /// Server-assigned request identifier, or empty.
    const std::string& request_id() const noexcept { return request_id_; }

    /// Raw response body, or empty when none was read.
    const std::string& raw() const noexcept { return raw_; }

    /// Uncapped Retry-After value in seconds; non-zero only on a 429.
    ///
    /// The retry loop caps the delay it actually waits at 60 seconds; this
    /// value is the original server figure.
    int retry_after() const noexcept { return retry_after_; }

    /// Returns a formatted, human-readable description of the error.
    const char* what() const noexcept override { return what_.c_str(); }

private:
    ErrorKind kind_;
    int status_;
    std::string code_;
    std::string message_;
    std::string request_id_;
    std::string raw_;
    int retry_after_;
    std::string what_;
};

} // namespace skailar

#endif // SKAILAR_ERRORS_HPP
