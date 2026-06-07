#ifndef SKAILAR_TESTS_TEST_SUPPORT_HPP
#define SKAILAR_TESTS_TEST_SUPPORT_HPP

#include "mock_server.hpp"

#include <skailar/skailar.hpp>

#include <cstdlib>
#include <string>

namespace skailar::testing {

/// A syntactically valid test API key (never sent to a real gateway).
inline constexpr const char* kTestKey = "skl_live_0123456789012345678901234567890123456789012";

/// Sets an environment variable, portably across POSIX and Windows.
inline void set_env(const char* name, const char* value) {
#ifdef _WIN32
    _putenv_s(name, value);
#else
    ::setenv(name, value, 1);
#endif
}

/// Clears an environment variable, portably across POSIX and Windows.
inline void unset_env(const char* name) {
#ifdef _WIN32
    _putenv_s(name, "");
#else
    ::unsetenv(name);
#endif
}

/// Builds a Client pointed at a MockServer with the test key and no retries.
inline Client make_client(const MockServer& server, int max_retries = 0) {
    ClientConfig cfg;
    cfg.api_key = kTestKey;
    cfg.base_url = server.base_url();
    cfg.max_retries = max_retries;
    return Client(cfg);
}

/// A minimal valid chat-completion JSON body with the given assistant text.
inline std::string sample_completion(const std::string& text) {
    nlohmann::json j = {
        {"id", "cmpl-test"},
        {"object", "chat.completion"},
        {"created", 1700000000},
        {"model", "test-model"},
        {"choices",
         nlohmann::json::array({{
             {"index", 0},
             {"message", {{"role", "assistant"}, {"content", text}}},
             {"finish_reason", "stop"},
         }})},
        {"usage", {{"prompt_tokens", 3}, {"completion_tokens", 5}, {"total_tokens", 8}}},
    };
    return j.dump();
}

/// A Skailar-shaped error body.
inline std::string sample_error(const std::string& type, const std::string& message) {
    nlohmann::json j = {{"error", {{"type", type}, {"message", message}}}};
    return j.dump();
}

} // namespace skailar::testing

#endif // SKAILAR_TESTS_TEST_SUPPORT_HPP
