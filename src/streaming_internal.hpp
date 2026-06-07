#ifndef SKAILAR_STREAMING_INTERNAL_HPP
#define SKAILAR_STREAMING_INTERNAL_HPP

#include "http_client.hpp"

#include <skailar/streaming.hpp>

#include <memory>
#include <string>

namespace skailar::detail {

/// Opens a streaming chat completion: connects, validates the response status,
/// and returns a stream that pumps SSE events on demand. @throws skailar::Error.
std::unique_ptr<ChatCompletionStream>
open_chat_stream(const HttpContext& ctx, const std::string& path, const std::string& body);

} // namespace skailar::detail

#endif // SKAILAR_STREAMING_INTERNAL_HPP
