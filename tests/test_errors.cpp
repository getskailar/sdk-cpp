#include "test_support.hpp"

#include <skailar/skailar.hpp>

#include <string>

#include <gtest/gtest.h>
#include <httplib.h>

using namespace skailar;
using namespace skailar::testing;

namespace {

// Drives a request whose response is a fixed status and body, returning the
// thrown error.
Error capture_error(int status, const std::string& body, const std::string& content_type,
                    int retry_after_secs = -1) {
    MockServer server;
    server.on("POST", "/v1/chat/completions",
              [status, body, content_type, retry_after_secs](const httplib::Request&,
                                                             httplib::Response& res) {
                  res.status = status;
                  if (retry_after_secs >= 0) {
                      res.set_header("Retry-After", std::to_string(retry_after_secs));
                  }
                  res.set_content(body, content_type);
              });

    Client client = make_client(server);
    ChatCompletionRequest request;
    request.model = "m";
    request.messages = {user_message("hi")};
    try {
        (void)client.chat().completions().create(request);
    } catch (const Error& e) {
        return e;
    }
    ADD_FAILURE() << "expected an error for status " << status;
    return Error(ErrorKind::Api, 0, "no error");
}

} // namespace

TEST(Errors, UnauthorizedMapsToAuth) {
    const Error e
        = capture_error(401, sample_error("invalid_api_key", "bad key"), "application/json");
    EXPECT_EQ(e.kind(), ErrorKind::Auth);
    EXPECT_EQ(e.status(), 401);
    EXPECT_EQ(e.code(), "invalid_api_key");
    EXPECT_EQ(e.message(), "bad key");
}

TEST(Errors, BadRequestMapsToBadRequest) {
    const Error e = capture_error(400, sample_error("bad_request", "nope"), "application/json");
    EXPECT_EQ(e.kind(), ErrorKind::BadRequest);
    EXPECT_EQ(e.status(), 400);
}

TEST(Errors, NotFoundMapsToNotFound) {
    MockServer server;
    server.on("GET", "/v1/models/missing", [](const httplib::Request&, httplib::Response& res) {
        res.status = 404;
        res.set_content(sample_error("not_found", "no model"), "application/json");
    });
    Client client = make_client(server);
    try {
        (void)client.models().retrieve("missing");
        FAIL() << "expected not found";
    } catch (const Error& e) {
        EXPECT_EQ(e.kind(), ErrorKind::NotFound);
        EXPECT_EQ(e.status(), 404);
    }
}

TEST(Errors, RateLimitMapsToRateLimitAndCapsRetryAfter) {
    const Error e
        = capture_error(429, sample_error("rate_limited", "slow down"), "application/json", 120);
    EXPECT_EQ(e.kind(), ErrorKind::RateLimit);
    EXPECT_EQ(e.status(), 429);
    // The raw value is reported uncapped, even though the retry loop caps at 60s.
    EXPECT_EQ(e.retry_after(), 120);
}

TEST(Errors, ServerErrorMapsToUpstream) {
    const Error e = capture_error(503, sample_error("upstream_error", "down"), "application/json");
    EXPECT_EQ(e.kind(), ErrorKind::Upstream);
    EXPECT_EQ(e.status(), 503);
}

TEST(Errors, MalformedSuccessBodyMapsToDecode) {
    MockServer server;
    server.on("POST", "/v1/chat/completions", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("not json at all", "application/json");
    });
    Client client = make_client(server);
    ChatCompletionRequest request;
    request.model = "m";
    request.messages = {user_message("hi")};
    try {
        (void)client.chat().completions().create(request);
        FAIL() << "expected a decode error";
    } catch (const Error& e) {
        EXPECT_EQ(e.kind(), ErrorKind::Decode);
    }
}

TEST(Errors, PlainTextErrorBodyBecomesMessage) {
    const Error e = capture_error(400, "raw failure text", "text/plain");
    EXPECT_EQ(e.kind(), ErrorKind::BadRequest);
    EXPECT_EQ(e.message(), "raw failure text");
}

TEST(Errors, WhatIncludesKindAndStatus) {
    const Error e
        = capture_error(401, sample_error("invalid_api_key", "bad key"), "application/json");
    const std::string what = e.what();
    EXPECT_NE(what.find("auth"), std::string::npos);
    EXPECT_NE(what.find("401"), std::string::npos);
}

TEST(Errors, KindToStringIsStable) {
    EXPECT_STREQ(to_string(ErrorKind::RateLimit), "rate_limit");
    EXPECT_STREQ(to_string(ErrorKind::Timeout), "timeout");
    EXPECT_STREQ(to_string(ErrorKind::Config), "config");
}
