#ifndef SKAILAR_CONFIG_HPP
#define SKAILAR_CONFIG_HPP

#include <chrono>
#include <map>
#include <string>

namespace skailar {

/// Configuration for a @ref Client. Every member has a default; construct one,
/// set the fields you need, and pass it to the @ref Client constructor.
struct ClientConfig {
    /// API key. When empty, the client reads the `SKAILAR_API_KEY` environment
    /// variable; if that is also empty, construction throws @ref ErrorKind::Config.
    std::string api_key;

    /// Base URL. When empty, the client reads `SKAILAR_BASE_URL`, falling back
    /// to `https://api.skailar.com`. A trailing slash is trimmed.
    std::string base_url;

    /// Per-request timeout. The default is 60 seconds. A non-positive value
    /// disables the SDK-managed timeout.
    std::chrono::milliseconds timeout {60000};

    /// Maximum number of retries for eligible requests. The default is 2 (three
    /// attempts total). A negative value is clamped to 0.
    int max_retries = 2;

    /// Headers sent on every request. An `Authorization` header set here is
    /// ignored; the SDK always applies its own bearer token.
    std::map<std::string, std::string> default_headers;
};

} // namespace skailar

#endif // SKAILAR_CONFIG_HPP
