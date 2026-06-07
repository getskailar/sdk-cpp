#include <skailar/client.hpp>

#include "client_impl.hpp"

#include <skailar/audio.hpp>

#include <string>

namespace skailar {

TranscriptionResponse TranscriptionsResource::create(const TranscriptionRequest& request) const {
    return client_->impl()->post_json<TranscriptionRequest, TranscriptionResponse>(
        "v1/audio/transcriptions", request);
}

std::string SpeechResource::create(const SpeechRequest& request) const {
    return client_->impl()->post_binary("v1/audio/speech", request);
}

} // namespace skailar
