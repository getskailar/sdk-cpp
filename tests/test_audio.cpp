#include "test_support.hpp"

#include <skailar/skailar.hpp>

#include <string>

#include <gtest/gtest.h>
#include <httplib.h>

using namespace skailar;
using namespace skailar::testing;

TEST(Audio, TranscriptionReturnsText) {
    MockServer server;
    server.on("POST", "/v1/audio/transcriptions",
              [](const httplib::Request& req, httplib::Response& res) {
                  const auto body = nlohmann::json::parse(req.body);
                  EXPECT_EQ(body.at("base64"), "AAAA");
                  EXPECT_EQ(body.at("mime"), "audio/mpeg");
                  res.set_content(R"({"text":"hello world"})", "application/json");
              });

    Client client = make_client(server);
    TranscriptionRequest request;
    request.base64 = "AAAA";
    request.mime = Mime::Mpeg;
    const TranscriptionResponse resp = client.audio().transcriptions().create(request);
    EXPECT_EQ(resp.text, "hello world");
}

TEST(Audio, TranscriptionOmitsMimeWhenUnset) {
    MockServer server;
    std::string captured;
    server.on("POST", "/v1/audio/transcriptions",
              [&captured](const httplib::Request& req, httplib::Response& res) {
                  captured = req.body;
                  res.set_content(R"({"text":"x"})", "application/json");
              });

    Client client = make_client(server);
    TranscriptionRequest request;
    request.base64 = "BBBB";
    (void)client.audio().transcriptions().create(request);

    const auto body = nlohmann::json::parse(captured);
    EXPECT_FALSE(body.contains("mime"));
}

TEST(Audio, SpeechReturnsBinaryBytes) {
    const std::string fake_mp3 = std::string("ID3\x04") + std::string(64, '\x01');
    MockServer server;
    server.on("POST", "/v1/audio/speech",
              [&fake_mp3](const httplib::Request& req, httplib::Response& res) {
                  const auto body = nlohmann::json::parse(req.body);
                  EXPECT_EQ(body.at("input"), "say this");
                  EXPECT_EQ(body.at("voice"), "nova");
                  res.set_content(fake_mp3, "audio/mpeg");
              });

    Client client = make_client(server);
    SpeechRequest request;
    request.input = "say this";
    request.voice = Voice::Nova;
    const std::string audio = client.audio().speech().create(request);
    EXPECT_EQ(audio, fake_mp3);
    EXPECT_EQ(audio.size(), fake_mp3.size());
}

TEST(Audio, SpeechOmitsVoiceWhenUnset) {
    MockServer server;
    std::string captured;
    server.on("POST", "/v1/audio/speech",
              [&captured](const httplib::Request& req, httplib::Response& res) {
                  captured = req.body;
                  res.set_content("audio", "audio/mpeg");
              });

    Client client = make_client(server);
    SpeechRequest request;
    request.input = "hi";
    (void)client.audio().speech().create(request);

    const auto body = nlohmann::json::parse(captured);
    EXPECT_FALSE(body.contains("voice"));
}
