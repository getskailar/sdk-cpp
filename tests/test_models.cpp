#include "test_support.hpp"

#include <skailar/skailar.hpp>

#include <string>

#include <gtest/gtest.h>
#include <httplib.h>

using namespace skailar;
using namespace skailar::testing;

TEST(Models, ListReturnsCatalog) {
    MockServer server;
    server.on("GET", "/v1/models", [](const httplib::Request&, httplib::Response& res) {
        nlohmann::json j = {
            {"object", "list"},
            {"data",
             nlohmann::json::array({{
                 {"id", "claude-sonnet-4-6"},
                 {"object", "model"},
                 {"created", 1},
                 {"owned_by", "anthropic"},
                 {"display_name", "Claude Sonnet 4.6"},
                 {"context_window", 200000},
                 {"max_output_tokens", 8192},
                 {"capabilities",
                  {{"streaming", true},
                   {"tool_calls", true},
                   {"vision", true},
                   {"json_mode", true}}},
                 {"pricing",
                  {{"input_per_mtok", 3.0}, {"output_per_mtok", 15.0}, {"currency", "USD"}}},
                 {"status", "active"},
             }})},
        };
        res.set_content(j.dump(), "application/json");
    });

    Client client = make_client(server);
    const ModelList list = client.models().list();
    ASSERT_EQ(list.data.size(), 1U);
    EXPECT_EQ(list.data.at(0).id, "claude-sonnet-4-6");
    EXPECT_EQ(list.data.at(0).owned_by, "anthropic");
    EXPECT_TRUE(list.data.at(0).capabilities.vision);
    EXPECT_DOUBLE_EQ(list.data.at(0).pricing.input_per_mtok, 3.0);
}

TEST(Models, RetrieveReturnsDetail) {
    MockServer server;
    server.on("GET", "/v1/models/gpt-5", [](const httplib::Request&, httplib::Response& res) {
        nlohmann::json j = {
            {"id", "gpt-5"},
            {"object", "model"},
            {"created", 1},
            {"owned_by", "openai"},
            {"display_name", "GPT-5"},
            {"context_window", 400000},
            {"max_output_tokens", 16384},
            {"capabilities",
             {{"streaming", true},
              {"tool_calls", true},
              {"vision", true},
              {"json_mode", true},
              {"reasoning", true}}},
            {"pricing", {{"input_per_mtok", 1.25}, {"output_per_mtok", 10.0}, {"currency", "USD"}}},
            {"status", "active"},
            {"description", "Flagship model."},
            {"aliases", nlohmann::json::array({"gpt5"})},
        };
        res.set_content(j.dump(), "application/json");
    });

    Client client = make_client(server);
    const Model model = client.models().retrieve("gpt-5");
    EXPECT_EQ(model.id, "gpt-5");
    ASSERT_TRUE(model.description.has_value());
    EXPECT_EQ(*model.description, "Flagship model.");
    ASSERT_TRUE(model.capabilities.reasoning.has_value());
    EXPECT_TRUE(*model.capabilities.reasoning);
    ASSERT_TRUE(model.aliases.has_value());
    EXPECT_EQ(model.aliases->at(0), "gpt5");
}

TEST(Models, RetrieveEncodesProviderPrefixedId) {
    MockServer server;
    std::string seen_path;
    server.on("GET", "/v1/models/google/gemini-2.5-pro",
              [&seen_path](const httplib::Request& req, httplib::Response& res) {
                  seen_path = req.path;
                  nlohmann::json j = {
                      {"id", "google/gemini-2.5-pro"},
                      {"object", "model"},
                      {"created", 1},
                      {"owned_by", "google"},
                      {"display_name", "Gemini 2.5 Pro"},
                      {"context_window", 1000000},
                      {"max_output_tokens", 8192},
                      {"capabilities",
                       {{"streaming", true},
                        {"tool_calls", true},
                        {"vision", true},
                        {"json_mode", true}}},
                      {"pricing",
                       {{"input_per_mtok", 1.0}, {"output_per_mtok", 5.0}, {"currency", "USD"}}},
                      {"status", "active"},
                  };
                  res.set_content(j.dump(), "application/json");
              });

    Client client = make_client(server);
    const Model model = client.models().retrieve("google/gemini-2.5-pro");
    EXPECT_EQ(model.id, "google/gemini-2.5-pro");
    EXPECT_EQ(seen_path, "/v1/models/google/gemini-2.5-pro");
}
