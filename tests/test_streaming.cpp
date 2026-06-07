#include "test_support.hpp"

#include <skailar/skailar.hpp>

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <httplib.h>

using namespace skailar;
using namespace skailar::testing;

namespace {

std::string chunk_event(const std::string& content, bool final_chunk) {
    nlohmann::json delta = {{"content", content}};
    nlohmann::json choice = {{"index", 0}, {"delta", delta}};
    if (final_chunk) {
        choice["finish_reason"] = "stop";
    }
    nlohmann::json j = {
        {"id", "chunk-1"}, {"object", "chat.completion.chunk"},          {"created", 1},
        {"model", "m"},    {"choices", nlohmann::json::array({choice})},
    };
    return "data: " + j.dump() + "\n\n";
}

void serve_sse(httplib::Response& res, std::string payload) {
    res.set_content(std::move(payload), "text/event-stream");
}

} // namespace

TEST(Streaming, YieldsChunksThenEnds) {
    MockServer server;
    server.on("POST", "/v1/chat/completions",
              [](const httplib::Request& req, httplib::Response& res) {
                  EXPECT_EQ(req.get_header_value("Accept"), "text/event-stream");
                  std::string body = chunk_event("Hello", false) + chunk_event(" world", true)
                      + "data: [DONE]\n\n";
                  serve_sse(res, std::move(body));
              });

    Client client = make_client(server);
    ChatCompletionRequest request;
    request.model = "m";
    request.messages = {user_message("hi")};

    auto stream = client.chat().completions().create_stream(request);
    std::string text;
    int count = 0;
    while (auto chunk = stream->next()) {
        if (auto delta = chunk->content_delta()) {
            text += *delta;
        }
        ++count;
    }
    EXPECT_EQ(text, "Hello world");
    EXPECT_EQ(count, 2);
}

TEST(Streaming, SetsStreamTrueInBody) {
    MockServer server;
    std::string captured;
    server.on("POST", "/v1/chat/completions",
              [&captured](const httplib::Request& req, httplib::Response& res) {
                  captured = req.body;
                  serve_sse(res, "data: [DONE]\n\n");
              });

    Client client = make_client(server);
    ChatCompletionRequest request;
    request.model = "m";
    request.messages = {user_message("hi")};
    auto stream = client.chat().completions().create_stream(request);
    while (stream->next()) { }

    const auto body = nlohmann::json::parse(captured);
    EXPECT_EQ(body.at("stream"), true);
}

TEST(Streaming, InBandErrorFrameThrows) {
    MockServer server;
    server.on("POST", "/v1/chat/completions", [](const httplib::Request&, httplib::Response& res) {
        std::string body = chunk_event("partial", false)
            + "data: {\"error\":{\"type\":\"upstream_error\",\"message\":\"boom\"}}\n\n";
        serve_sse(res, std::move(body));
    });

    Client client = make_client(server);
    ChatCompletionRequest request;
    request.model = "m";
    request.messages = {user_message("hi")};

    auto stream = client.chat().completions().create_stream(request);
    bool threw = false;
    try {
        while (stream->next()) { }
    } catch (const Error& e) {
        threw = true;
        EXPECT_EQ(e.kind(), ErrorKind::Upstream);
        EXPECT_EQ(e.message(), "boom");
    }
    EXPECT_TRUE(threw);
}

TEST(Streaming, ErrorStatusBeforeStreamThrows) {
    MockServer server;
    server.on("POST", "/v1/chat/completions", [](const httplib::Request&, httplib::Response& res) {
        res.status = 401;
        res.set_content(sample_error("invalid_api_key", "bad key"), "application/json");
    });

    Client client = make_client(server);
    ChatCompletionRequest request;
    request.model = "m";
    request.messages = {user_message("hi")};

    try {
        auto stream = client.chat().completions().create_stream(request);
        while (stream->next()) { }
        FAIL() << "expected an auth error";
    } catch (const Error& e) {
        EXPECT_EQ(e.kind(), ErrorKind::Auth);
        EXPECT_EQ(e.status(), 401);
    }
}

TEST(Streaming, CloseIsIdempotent) {
    MockServer server;
    server.on("POST", "/v1/chat/completions", [](const httplib::Request&, httplib::Response& res) {
        serve_sse(res, chunk_event("x", true) + "data: [DONE]\n\n");
    });

    Client client = make_client(server);
    ChatCompletionRequest request;
    request.model = "m";
    request.messages = {user_message("hi")};
    auto stream = client.chat().completions().create_stream(request);
    stream->close();
    stream->close();
    EXPECT_FALSE(stream->next().has_value());
}
