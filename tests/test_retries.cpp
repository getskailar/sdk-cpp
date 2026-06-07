#include "test_support.hpp"

#include <skailar/skailar.hpp>

#include <atomic>
#include <string>

#include <gtest/gtest.h>
#include <httplib.h>

using namespace skailar;
using namespace skailar::testing;

TEST(Retries, GetRetriesOnServerErrorThenSucceeds) {
    MockServer server;
    std::atomic<int> calls {0};
    server.on("GET", "/v1/models", [&calls](const httplib::Request&, httplib::Response& res) {
        const int n = ++calls;
        if (n < 3) {
            res.status = 503;
            res.set_content(sample_error("upstream_error", "down"), "application/json");
        } else {
            res.set_content(R"({"object":"list","data":[]})", "application/json");
        }
    });

    Client client = make_client(server, /*max_retries=*/3);
    const ModelList list = client.models().list();
    EXPECT_EQ(list.object, "list");
    EXPECT_EQ(calls.load(), 3);
}

TEST(Retries, GetStopsAfterMaxRetries) {
    MockServer server;
    std::atomic<int> calls {0};
    server.on("GET", "/v1/models", [&calls](const httplib::Request&, httplib::Response& res) {
        ++calls;
        res.status = 500;
        res.set_content(sample_error("upstream_error", "down"), "application/json");
    });

    Client client = make_client(server, /*max_retries=*/2);
    EXPECT_THROW((void)client.models().list(), Error);
    // 1 initial attempt + 2 retries.
    EXPECT_EQ(calls.load(), 3);
}

TEST(Retries, PostIsNotRetriedOnServerError) {
    MockServer server;
    std::atomic<int> calls {0};
    server.on("POST", "/v1/chat/completions",
              [&calls](const httplib::Request&, httplib::Response& res) {
                  ++calls;
                  res.status = 500;
                  res.set_content(sample_error("upstream_error", "down"), "application/json");
              });

    Client client = make_client(server, /*max_retries=*/5);
    ChatCompletionRequest request;
    request.model = "m";
    request.messages = {user_message("hi")};
    try {
        (void)client.chat().completions().create(request);
        FAIL() << "expected an upstream error";
    } catch (const Error& e) {
        EXPECT_EQ(e.kind(), ErrorKind::Upstream);
    }
    // A side-effecting POST must not be replayed on 5xx, to avoid double billing.
    EXPECT_EQ(calls.load(), 1);
}

TEST(Retries, PostIsRetriedOnRateLimit) {
    MockServer server;
    std::atomic<int> calls {0};
    server.on("POST", "/v1/chat/completions",
              [&calls](const httplib::Request&, httplib::Response& res) {
                  const int n = ++calls;
                  if (n < 2) {
                      res.status = 429;
                      res.set_header("Retry-After", "0");
                      res.set_content(sample_error("rate_limited", "slow"), "application/json");
                  } else {
                      res.set_content(sample_completion("ok"), "application/json");
                  }
              });

    Client client = make_client(server, /*max_retries=*/3);
    ChatCompletionRequest request;
    request.model = "m";
    request.messages = {user_message("hi")};
    const ChatCompletionResponse resp = client.chat().completions().create(request);
    EXPECT_EQ(resp.choices.at(0).message.content.text(), "ok");
    EXPECT_EQ(calls.load(), 2);
}

TEST(Retries, ZeroRetriesMeansSingleAttempt) {
    MockServer server;
    std::atomic<int> calls {0};
    server.on("GET", "/v1/models", [&calls](const httplib::Request&, httplib::Response& res) {
        ++calls;
        res.status = 500;
        res.set_content(sample_error("upstream_error", "down"), "application/json");
    });

    Client client = make_client(server, /*max_retries=*/0);
    EXPECT_THROW((void)client.models().list(), Error);
    EXPECT_EQ(calls.load(), 1);
}
