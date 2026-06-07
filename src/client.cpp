#include <skailar/client.hpp>

#include "client_impl.hpp"

#include <skailar/config.hpp>
#include <skailar/errors.hpp>
#include <skailar/ping.hpp>

#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

namespace skailar {

namespace {

constexpr const char* kEnvApiKey = "SKAILAR_API_KEY";
constexpr const char* kEnvBaseUrl = "SKAILAR_BASE_URL";
constexpr const char* kDefaultBaseUrl = "https://api.skailar.com";

std::string env_or_empty(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr ? std::string(value) : std::string();
}

void trim_trailing_slashes(std::string& url) {
    while (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
}

// Resolves and validates the configuration, applying environment fallbacks.
ClientConfig resolve(ClientConfig config) {
    if (config.api_key.empty()) {
        config.api_key = env_or_empty(kEnvApiKey);
    }
    if (config.api_key.empty()) {
        throw Error(ErrorKind::Config, 0,
                    std::string("missing API key (set ClientConfig::api_key or the ") + kEnvApiKey
                        + " environment variable)");
    }

    if (config.base_url.empty()) {
        config.base_url = env_or_empty(kEnvBaseUrl);
    }
    if (config.base_url.empty()) {
        config.base_url = kDefaultBaseUrl;
    }
    trim_trailing_slashes(config.base_url);

    if (config.base_url.rfind("http://", 0) != 0 && config.base_url.rfind("https://", 0) != 0) {
        throw Error(ErrorKind::Config, 0,
                    "invalid base URL (must begin with http:// or https://): " + config.base_url);
    }

    if (config.max_retries < 0) {
        config.max_retries = 0;
    }
    return config;
}

} // namespace

Client::Client() : Client(ClientConfig {}) { }

Client::Client(ClientConfig config) : impl_(std::make_unique<Impl>(resolve(std::move(config)))) { }

Client::~Client() = default;
Client::Client(Client&&) noexcept = default;
Client& Client::operator=(Client&&) noexcept = default;

PingKeyResponse Client::ping() {
    return impl_->get_json<PingKeyResponse>("v1/ping-key");
}

} // namespace skailar
