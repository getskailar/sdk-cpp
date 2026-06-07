#include "test_support.hpp"

#include <skailar/skailar.hpp>

#include <string>

#include <gtest/gtest.h>
#include <httplib.h>

using namespace skailar;
using namespace skailar::testing;

TEST(ChatCompletions, CreateReturnsMessage) {
    MockServer server;
    server.on(
        "POST", "/v1/chat/completions", [](const httplib::Request& req, httplib::Response& res) {
            EXPECT_EQ(req.get_header_value("Authorization"), std::string("Bearer ") + kTestKey);
            res.set_content(sample_completion("Hi!"), "application/json");
        });

    Client client = make_client(server);
    ChatCompletionRequest request;
    request.model = "m";
    request.messages = {user_message("hi")};

    const ChatCompletionResponse resp = client.chat().completions().create(request);
    EXPECT_EQ(resp.choices.at(0).message.content.text(), "Hi!");
    EXPECT_EQ(resp.choices.at(0).finish_reason, FinishReason::Stop);
    EXPECT_EQ(resp.usage.total_tokens, 8);
}

TEST(ChatCompletions, SendsModelAndMessagesInBody) {
    MockServer server;
    std::string captured_body;
    server.on("POST", "/v1/chat/completions",
              [&captured_body](const httplib::Request& req, httplib::Response& res) {
                  captured_body = req.body;
                  res.set_content(sample_completion("ok"), "application/json");
              });

    Client client = make_client(server);
    ChatCompletionRequest request;
    request.model = std::string {models::claude_sonnet_4_6};
    request.messages = {system_message("be brief"), user_message("hello")};
    request.max_tokens = 64;
    (void)client.chat().completions().create(request);

    const auto body = nlohmann::json::parse(captured_body);
    EXPECT_EQ(body.at("model"), "claude-sonnet-4-6");
    EXPECT_EQ(body.at("messages").size(), 2U);
    EXPECT_EQ(body.at("messages").at(0).at("role"), "system");
    EXPECT_EQ(body.at("messages").at(1).at("content"), "hello");
    EXPECT_EQ(body.at("max_tokens"), 64);
}

TEST(ChatCompletions, OmitsUnsetOptionalFields) {
    MockServer server;
    std::string captured_body;
    server.on("POST", "/v1/chat/completions",
              [&captured_body](const httplib::Request& req, httplib::Response& res) {
                  captured_body = req.body;
                  res.set_content(sample_completion("ok"), "application/json");
              });

    Client client = make_client(server);
    ChatCompletionRequest request;
    request.model = "m";
    request.messages = {user_message("hi")};
    (void)client.chat().completions().create(request);

    const auto body = nlohmann::json::parse(captured_body);
    EXPECT_FALSE(body.contains("temperature"));
    EXPECT_FALSE(body.contains("max_tokens"));
    EXPECT_FALSE(body.contains("tools"));
    EXPECT_FALSE(body.contains("stop"));
}

TEST(ChatCompletions, SerializesToolChoiceAndTools) {
    MockServer server;
    std::string captured_body;
    server.on("POST", "/v1/chat/completions",
              [&captured_body](const httplib::Request& req, httplib::Response& res) {
                  captured_body = req.body;
                  res.set_content(sample_completion("ok"), "application/json");
              });

    Client client = make_client(server);
    ChatCompletionRequest request;
    request.model = "m";
    request.messages = {user_message("weather?")};
    request.tools = std::vector<Tool> {
        function_tool("get_weather", "Get weather", nlohmann::json {{"type", "object"}})};
    request.tool_choice = ToolChoice::named("get_weather");
    (void)client.chat().completions().create(request);

    const auto body = nlohmann::json::parse(captured_body);
    ASSERT_TRUE(body.contains("tools"));
    EXPECT_EQ(body.at("tools").at(0).at("function").at("name"), "get_weather");
    EXPECT_EQ(body.at("tool_choice").at("type"), "function");
    EXPECT_EQ(body.at("tool_choice").at("function").at("name"), "get_weather");
}

TEST(ChatCompletions, ParsesToolCallsInResponse) {
    MockServer server;
    server.on("POST", "/v1/chat/completions", [](const httplib::Request&, httplib::Response& res) {
        nlohmann::json j = {
            {"id", "x"},
            {"object", "chat.completion"},
            {"created", 1},
            {"model", "m"},
            {"choices",
             nlohmann::json::array({{
                 {"index", 0},
                 {"message",
                  {{"role", "assistant"},
                   {"content", nullptr},
                   {"tool_calls",
                    nlohmann::json::array({{
                        {"id", "call_1"},
                        {"type", "function"},
                        {"function",
                         {{"name", "get_weather"}, {"arguments", "{\"city\":\"NYC\"}"}}},
                    }})}}},
                 {"finish_reason", "tool_calls"},
             }})},
            {"usage", {{"prompt_tokens", 1}, {"completion_tokens", 1}, {"total_tokens", 2}}},
        };
        res.set_content(j.dump(), "application/json");
    });

    Client client = make_client(server);
    ChatCompletionRequest request;
    request.model = "m";
    request.messages = {user_message("weather in NYC?")};

    const ChatCompletionResponse resp = client.chat().completions().create(request);
    EXPECT_EQ(resp.choices.at(0).finish_reason, FinishReason::ToolCalls);
    ASSERT_TRUE(resp.choices.at(0).message.tool_calls.has_value());
    const auto& calls = *resp.choices.at(0).message.tool_calls;
    ASSERT_EQ(calls.size(), 1U);
    EXPECT_EQ(calls.at(0).id, "call_1");
    EXPECT_EQ(calls.at(0).function.name, "get_weather");
}

TEST(ChatCompletions, MultimodalContentRoundTrips) {
    ChatMessage message;
    message.role = Role::User;
    message.content = MessageContent::parts({text_part("describe"), image_part("https://x/y.png")});

    const nlohmann::json j = message;
    ASSERT_TRUE(j.at("content").is_array());
    EXPECT_EQ(j.at("content").at(0).at("type"), "text");
    EXPECT_EQ(j.at("content").at(1).at("type"), "image_url");
    EXPECT_EQ(j.at("content").at(1).at("image_url").at("url"), "https://x/y.png");

    const auto parsed = j.get<ChatMessage>();
    ASSERT_TRUE(parsed.content.has_value());
    EXPECT_TRUE(parsed.content->is_parts());
    EXPECT_EQ(parsed.content->text(), "describe");
}

TEST(ChatCompletions, StopSequenceSerializesBothShapes) {
    nlohmann::json one = StopSequence::one("END");
    EXPECT_TRUE(one.is_string());
    EXPECT_EQ(one, "END");

    nlohmann::json many = StopSequence::many({"A", "B"});
    EXPECT_TRUE(many.is_array());
    EXPECT_EQ(many.size(), 2U);
}

TEST(ChatCompletions, ReasoningEffortSerializes) {
    ChatCompletionRequest request;
    request.model = "m";
    request.messages = {user_message("hi")};
    request.reasoning_effort = ReasoningEffort::High;

    const nlohmann::json j = request;
    EXPECT_EQ(j.at("reasoning_effort"), "high");
}
