#ifndef SKAILAR_AUDIO_HPP
#define SKAILAR_AUDIO_HPP

#include <skailar/json_fwd.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace skailar {

/// The MIME type of an audio clip submitted for transcription.
enum class Mime : std::uint8_t {
    /// audio/wav.
    Wav,
    /// audio/webm.
    Webm,
    /// audio/mp4.
    Mp4,
    /// audio/m4a.
    M4a,
    /// audio/mpeg.
    Mpeg,
    /// audio/mp3.
    Mp3,
};

/// Returns the wire string for @p mime (for example "audio/wav").
const char* to_string(Mime mime) noexcept;

/// A request to transcribe audio to text.
struct TranscriptionRequest {
    /// Base64-encoded audio bytes, without a `data:` prefix.
    std::string base64;
    /// Audio MIME type; defaults to audio/wav server-side when unset.
    std::optional<Mime> mime;
};

void to_json(json& j, const TranscriptionRequest& r);
void from_json(const json& j, TranscriptionRequest& r);

/// The response of @ref TranscriptionsResource::create.
struct TranscriptionResponse {
    /// The transcribed text.
    std::string text;
};

void to_json(json& j, const TranscriptionResponse& r);
void from_json(const json& j, TranscriptionResponse& r);

/// A synthesis voice for @ref SpeechResource::create.
enum class Voice : std::uint8_t {
    /// The "alloy" voice.
    Alloy,
    /// The "ash" voice.
    Ash,
    /// The "ballad" voice.
    Ballad,
    /// The "coral" voice.
    Coral,
    /// The "echo" voice.
    Echo,
    /// The "fable" voice.
    Fable,
    /// The "nova" voice (the default).
    Nova,
    /// The "onyx" voice.
    Onyx,
    /// The "sage" voice.
    Sage,
    /// The "shimmer" voice.
    Shimmer,
};

/// Returns the wire string for @p voice (for example "nova").
const char* to_string(Voice voice) noexcept;

/// A request to synthesize speech from text.
struct SpeechRequest {
    /// Text to synthesize, up to 4000 characters.
    std::string input;
    /// Synthesis voice; defaults to nova server-side when unset.
    std::optional<Voice> voice;
};

void to_json(json& j, const SpeechRequest& r);
void from_json(const json& j, SpeechRequest& r);

} // namespace skailar

#endif // SKAILAR_AUDIO_HPP
