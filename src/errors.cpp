#include <skailar/errors.hpp>

#include <string>
#include <utility>

namespace skailar {

const char* to_string(ErrorKind kind) noexcept {
    switch (kind) {
    case ErrorKind::Api:
        return "api";
    case ErrorKind::Auth:
        return "auth";
    case ErrorKind::BadRequest:
        return "bad_request";
    case ErrorKind::NotFound:
        return "not_found";
    case ErrorKind::RateLimit:
        return "rate_limit";
    case ErrorKind::Upstream:
        return "upstream";
    case ErrorKind::Network:
        return "network";
    case ErrorKind::Timeout:
        return "timeout";
    case ErrorKind::Aborted:
        return "aborted";
    case ErrorKind::Decode:
        return "decode";
    case ErrorKind::Config:
        return "config";
    }
    return "unknown";
}

namespace {

std::string format_what(ErrorKind kind, int status, const std::string& message) {
    std::string out = "skailar: ";
    out += to_string(kind);
    out += " error";
    if (status != 0) {
        out += " (status ";
        out += std::to_string(status);
        out += ")";
    }
    if (!message.empty()) {
        out += ": ";
        out += message;
    }
    return out;
}

} // namespace

Error::Error(ErrorKind kind, int status, std::string message, std::string code,
             std::string request_id, std::string raw, int retry_after)
    : kind_(kind), status_(status), code_(std::move(code)), message_(std::move(message)),
      request_id_(std::move(request_id)), raw_(std::move(raw)), retry_after_(retry_after),
      what_(format_what(kind, status, message_)) { }

} // namespace skailar
