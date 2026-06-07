#include "test_support.hpp"

#include <skailar/skailar.hpp>

#include <string>

#include <gtest/gtest.h>
#include <httplib.h>

using namespace skailar;
using namespace skailar::testing;

TEST(Images, GenerateReturnsImages) {
    MockServer server;
    server.on("POST", "/v1/images/generations",
              [](const httplib::Request& req, httplib::Response& res) {
                  const auto body = nlohmann::json::parse(req.body);
                  EXPECT_EQ(body.at("model"), "gpt-image-1");
                  EXPECT_EQ(body.at("prompt"), "a cat");
                  nlohmann::json j = {
                      {"created", 1},
                      {"data",
                       nlohmann::json::array(
                           {{{"url", "https://x/img.png"}, {"revised_prompt", "a fluffy cat"}}})},
                  };
                  res.set_content(j.dump(), "application/json");
              });

    Client client = make_client(server);
    ImageGenerationRequest request;
    request.model = std::string {models::gpt_image_1};
    request.prompt = "a cat";
    const ImageGenerationResponse resp = client.images().generate(request);
    ASSERT_EQ(resp.data.size(), 1U);
    ASSERT_TRUE(resp.data.at(0).url.has_value());
    EXPECT_EQ(*resp.data.at(0).url, "https://x/img.png");
    ASSERT_TRUE(resp.data.at(0).revised_prompt.has_value());
    EXPECT_EQ(*resp.data.at(0).revised_prompt, "a fluffy cat");
}

TEST(Images, OptionalFieldsSerialize) {
    MockServer server;
    std::string captured;
    server.on("POST", "/v1/images/generations",
              [&captured](const httplib::Request& req, httplib::Response& res) {
                  captured = req.body;
                  res.set_content(R"({"created":1,"data":[]})", "application/json");
              });

    Client client = make_client(server);
    ImageGenerationRequest request;
    request.model = "gpt-image-1";
    request.prompt = "x";
    request.n = 2;
    request.size = "1024x1024";
    request.quality = "hd";
    (void)client.images().generate(request);

    const auto body = nlohmann::json::parse(captured);
    EXPECT_EQ(body.at("n"), 2);
    EXPECT_EQ(body.at("size"), "1024x1024");
    EXPECT_EQ(body.at("quality"), "hd");
    EXPECT_FALSE(body.contains("background"));
}
