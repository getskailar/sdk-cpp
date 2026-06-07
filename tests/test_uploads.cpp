#include "test_support.hpp"

#include <skailar/skailar.hpp>

#include <string>

#include <gtest/gtest.h>
#include <httplib.h>

using namespace skailar;
using namespace skailar::testing;

TEST(Uploads, ImageUploadSendsContentTypeAndReturnsUrl) {
    MockServer server;
    server.on("POST", "/v1/uploads/images",
              [](const httplib::Request& req, httplib::Response& res) {
                  const auto body = nlohmann::json::parse(req.body);
                  EXPECT_EQ(body.at("base64"), "Zm9v");
                  EXPECT_EQ(body.at("content_type"), "image/png");
                  res.set_content(R"({"url":"skailar://img/1","content_type":"image/png"})",
                                  "application/json");
              });

    Client client = make_client(server);
    const UploadResponse resp = client.uploads().images().create("Zm9v", ImageContentType::Png);
    EXPECT_EQ(resp.url, "skailar://img/1");
    EXPECT_EQ(resp.content_type, "image/png");
}

TEST(Uploads, FileUploadSendsContentType) {
    MockServer server;
    server.on("POST", "/v1/uploads/files", [](const httplib::Request& req, httplib::Response& res) {
        const auto body = nlohmann::json::parse(req.body);
        EXPECT_EQ(body.at("content_type"), "application/pdf");
        res.set_content(R"({"url":"skailar://file/1","content_type":"application/pdf"})",
                        "application/json");
    });

    Client client = make_client(server);
    const UploadResponse resp = client.uploads().files().create("JVBERi0=", FileContentType::Pdf);
    EXPECT_EQ(resp.url, "skailar://file/1");
    EXPECT_EQ(resp.content_type, "application/pdf");
}

TEST(Uploads, ContentTypeEnumStrings) {
    EXPECT_STREQ(to_string(ImageContentType::Jpeg), "image/jpeg");
    EXPECT_STREQ(to_string(ImageContentType::Webp), "image/webp");
    EXPECT_STREQ(to_string(FileContentType::Text), "text/plain");
}
