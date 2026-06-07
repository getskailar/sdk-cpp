#include <skailar/audio.hpp>

#include "json_helpers.hpp"

#include <string>

#include <nlohmann/json.hpp>

namespace skailar {

namespace {

Mime mime_from_string(const std::string& s) {
    if (s == "audio/webm") {
        return Mime::Webm;
    }
    if (s == "audio/mp4") {
        return Mime::Mp4;
    }
    if (s == "audio/m4a") {
        return Mime::M4a;
    }
    if (s == "audio/mpeg") {
        return Mime::Mpeg;
    }
    if (s == "audio/mp3") {
        return Mime::Mp3;
    }
    return Mime::Wav;
}

Voice voice_from_string(const std::string& s) {
    if (s == "alloy") {
        return Voice::Alloy;
    }
    if (s == "ash") {
        return Voice::Ash;
    }
    if (s == "ballad") {
        return Voice::Ballad;
    }
    if (s == "coral") {
        return Voice::Coral;
    }
    if (s == "echo") {
        return Voice::Echo;
    }
    if (s == "fable") {
        return Voice::Fable;
    }
    if (s == "onyx") {
        return Voice::Onyx;
    }
    if (s == "sage") {
        return Voice::Sage;
    }
    if (s == "shimmer") {
        return Voice::Shimmer;
    }
    return Voice::Nova;
}

} // namespace

const char* to_string(Mime mime) noexcept {
    switch (mime) {
    case Mime::Wav:
        return "audio/wav";
    case Mime::Webm:
        return "audio/webm";
    case Mime::Mp4:
        return "audio/mp4";
    case Mime::M4a:
        return "audio/m4a";
    case Mime::Mpeg:
        return "audio/mpeg";
    case Mime::Mp3:
        return "audio/mp3";
    }
    return "audio/wav";
}

const char* to_string(Voice voice) noexcept {
    switch (voice) {
    case Voice::Alloy:
        return "alloy";
    case Voice::Ash:
        return "ash";
    case Voice::Ballad:
        return "ballad";
    case Voice::Coral:
        return "coral";
    case Voice::Echo:
        return "echo";
    case Voice::Fable:
        return "fable";
    case Voice::Nova:
        return "nova";
    case Voice::Onyx:
        return "onyx";
    case Voice::Sage:
        return "sage";
    case Voice::Shimmer:
        return "shimmer";
    }
    return "nova";
}

void to_json(json& j, const TranscriptionRequest& r) {
    j = json::object();
    j["base64"] = r.base64;
    if (r.mime.has_value()) {
        j["mime"] = to_string(*r.mime);
    }
}

void from_json(const json& j, TranscriptionRequest& r) {
    r.base64 = detail::get_string(j, "base64");
    if (auto it = j.find("mime"); it != j.end() && it->is_string()) {
        r.mime = mime_from_string(it->get<std::string>());
    } else {
        r.mime.reset();
    }
}

void to_json(json& j, const TranscriptionResponse& r) {
    j = json {{"text", r.text}};
}

void from_json(const json& j, TranscriptionResponse& r) {
    r.text = detail::get_string(j, "text");
}

void to_json(json& j, const SpeechRequest& r) {
    j = json::object();
    j["input"] = r.input;
    if (r.voice.has_value()) {
        j["voice"] = to_string(*r.voice);
    }
}

void from_json(const json& j, SpeechRequest& r) {
    r.input = detail::get_string(j, "input");
    if (auto it = j.find("voice"); it != j.end() && it->is_string()) {
        r.voice = voice_from_string(it->get<std::string>());
    } else {
        r.voice.reset();
    }
}

} // namespace skailar
