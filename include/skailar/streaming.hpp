#ifndef SKAILAR_STREAMING_HPP
#define SKAILAR_STREAMING_HPP

#include <skailar/chat.hpp>

#include <memory>
#include <optional>

namespace skailar {

/// A pull iterator over the @ref ChatCompletionChunk events of a streamed
/// completion.
///
/// Call @ref next repeatedly until it returns std::nullopt, which signals a
/// clean end of stream:
///
/// @code
/// auto stream = client.chat().completions().create_stream(req);
/// while (auto chunk = stream->next()) {
///     if (auto text = chunk->content_delta()) {
///         std::cout << *text << std::flush;
///     }
/// }
/// @endcode
///
/// The destructor closes the underlying connection. A stream is movable but not
/// copyable, and is not safe for concurrent use.
class ChatCompletionStream {
public:
    /// Internal pimpl; constructed by the SDK, not by callers.
    class Impl;

    /// Wraps an implementation. Used internally by the SDK.
    explicit ChatCompletionStream(std::unique_ptr<Impl> impl);

    ChatCompletionStream(ChatCompletionStream&&) noexcept;
    ChatCompletionStream& operator=(ChatCompletionStream&&) noexcept;
    ChatCompletionStream(const ChatCompletionStream&) = delete;
    ChatCompletionStream& operator=(const ChatCompletionStream&) = delete;

    /// Closes the underlying connection.
    ~ChatCompletionStream();

    /// Advances to the next chunk.
    ///
    /// @returns The next chunk, or std::nullopt at a clean end of stream.
    /// @throws skailar::Error on a transport failure, a malformed event, or an
    ///         in-band error frame mid-stream.
    std::optional<ChatCompletionChunk> next();

    /// Closes the underlying connection. Safe to call more than once and before
    /// the stream is fully consumed.
    void close() noexcept;

private:
    std::unique_ptr<Impl> impl_;
};

} // namespace skailar

#endif // SKAILAR_STREAMING_HPP
