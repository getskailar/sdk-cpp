#include "test_support.hpp"

#include <skailar/skailar.hpp>

#include <cstdlib>
#include <string>

#include <gtest/gtest.h>
#include <httplib.h>

using namespace skailar;
using namespace skailar::testing;

TEST(Client, MissingApiKeyThrowsConfig) {
    unset_env("SKAILAR_API_KEY");
    ClientConfig cfg;
    cfg.base_url = "http://127.0.0.1:1";
    try {
        Client client(cfg);
        FAIL() << "expected a config error";
    } catch (const Error& e) {
        EXPECT_EQ(e.kind(), ErrorKind::Config);
        EXPECT_EQ(e.status(), 0);
    }
}

TEST(Client, MalformedBaseUrlThrowsConfig) {
    ClientConfig cfg;
    cfg.api_key = kTestKey;
    cfg.base_url = "ftp://example.com";
    try {
        Client client(cfg);
        FAIL() << "expected a config error";
    } catch (const Error& e) {
        EXPECT_EQ(e.kind(), ErrorKind::Config);
    }
}

TEST(Client, ReadsApiKeyFromEnvironment) {
    MockServer server;
    server.on("GET", "/v1/ping-key", [](const httplib::Request& req, httplib::Response& res) {
        EXPECT_EQ(req.get_header_value("Authorization"), std::string("Bearer ") + kTestKey);
        res.set_content(R"({"status":"ok","user_id":"u-1"})", "application/json");
    });

    set_env("SKAILAR_API_KEY", kTestKey);
    ClientConfig cfg;
    cfg.base_url = server.base_url();
    Client client(cfg);
    const PingKeyResponse resp = client.ping();
    EXPECT_EQ(resp.status, "ok");
    EXPECT_EQ(resp.user_id, "u-1");
    unset_env("SKAILAR_API_KEY");
}

TEST(Client, DefaultHeaderIsSent) {
    MockServer server;
    server.on("GET", "/v1/ping-key", [](const httplib::Request& req, httplib::Response& res) {
        EXPECT_EQ(req.get_header_value("X-Trace-Id"), "abc123");
        res.set_content(R"({"status":"ok","user_id":"u"})", "application/json");
    });

    ClientConfig cfg;
    cfg.api_key = kTestKey;
    cfg.base_url = server.base_url();
    cfg.default_headers["X-Trace-Id"] = "abc123";
    Client client(cfg);
    (void)client.ping();
}

TEST(Client, DefaultAuthorizationHeaderCannotOverrideBearer) {
    MockServer server;
    std::string seen_auth;
    server.on("GET", "/v1/ping-key",
              [&seen_auth](const httplib::Request& req, httplib::Response& res) {
                  seen_auth = req.get_header_value("Authorization");
                  res.set_content(R"({"status":"ok","user_id":"u"})", "application/json");
              });

    ClientConfig cfg;
    cfg.api_key = kTestKey;
    cfg.base_url = server.base_url();
    cfg.default_headers["Authorization"] = "Bearer attacker-token";
    Client client(cfg);
    (void)client.ping();
    EXPECT_EQ(seen_auth, std::string("Bearer ") + kTestKey);
}

TEST(Client, AuthorizationHeaderDedupIsCaseInsensitive) {
    MockServer server;
    std::string seen_auth;
    server.on("GET", "/v1/ping-key",
              [&seen_auth](const httplib::Request& req, httplib::Response& res) {
                  seen_auth = req.get_header_value("Authorization");
                  // Exactly one Authorization header must reach the server.
                  EXPECT_EQ(req.get_header_value_count("Authorization"), 1U);
                  res.set_content(R"({"status":"ok","user_id":"u"})", "application/json");
              });

    ClientConfig cfg;
    cfg.api_key = kTestKey;
    cfg.base_url = server.base_url();
    cfg.default_headers["AUTHORIZATION"] = "Bearer upper";
    cfg.default_headers["authorization"] = "Bearer lower";
    Client client(cfg);
    (void)client.ping();
    EXPECT_EQ(seen_auth, std::string("Bearer ") + kTestKey);
}

TEST(Client, MoveLeavesUsableClient) {
    MockServer server;
    server.on("GET", "/v1/ping-key", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok","user_id":"u"})", "application/json");
    });

    Client original = make_client(server);
    Client moved = std::move(original);
    const PingKeyResponse resp = moved.ping();
    EXPECT_EQ(resp.status, "ok");
}

TEST(Client, PingReturnsUserId) {
    MockServer server;
    server.on("GET", "/v1/ping-key", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok","user_id":"user-42"})", "application/json");
    });

    Client client = make_client(server);
    EXPECT_EQ(client.ping().user_id, "user-42");
}
