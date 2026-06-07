#include <skailar/client.hpp>

#include "client_impl.hpp"
#include "streaming_internal.hpp"

#include <skailar/chat.hpp>
#include <skailar/streaming.hpp>

#include <memory>
#include <utility>

#include <nlohmann/json.hpp>

namespace skailar {

namespace {

constexpr const char* kChatCompletionsPath = "v1/chat/completions";

} // namespace

ChatCompletionResponse ChatCompletionsResource::create(const ChatCompletionRequest& request) const {
    return client_->impl()->post_json<ChatCompletionRequest, ChatCompletionResponse>(
        kChatCompletionsPath, request);
}

std::unique_ptr<ChatCompletionStream>
ChatCompletionsResource::create_stream(const ChatCompletionRequest& request) const {
    ChatCompletionRequest streamed = request;
    streamed.stream = true;
    std::string body = nlohmann::json(streamed).dump();
    return detail::open_chat_stream(client_->impl()->ctx(), kChatCompletionsPath, std::move(body));
}

} // namespace skailar
