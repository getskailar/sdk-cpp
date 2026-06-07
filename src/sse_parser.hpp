#ifndef SKAILAR_SSE_PARSER_HPP
#define SKAILAR_SSE_PARSER_HPP

#include <cstddef>
#include <cstdint>
#include <string>

namespace skailar::detail {

/// Incremental Server-Sent Events parser.
///
/// Bytes are appended with @ref feed as they arrive; complete `data:` event
/// payloads are pulled with @ref next_event. Line terminators `\n`, `\r\n`, and
/// `\r` are all accepted; blank lines and `:` comment lines are skipped.
/// Multiple `data:` lines within one event are joined with `\n` per the SSE
/// specification.
class SseParser {
public:
    /// Classifies the result of @ref next_event.
    enum class Status : std::uint8_t {
        /// An event payload is available in the out-parameter.
        Event,
        /// The `[DONE]` sentinel was seen; the stream is complete.
        Done,
        /// No complete event is buffered yet; feed more bytes.
        NeedMore,
    };

    /// Appends raw bytes from the transport.
    void feed(const char* data, std::size_t size);

    /// Signals that the transport has closed. A trailing line without a
    /// terminator is then treated as complete.
    void mark_eof() noexcept { eof_ = true; }

    /// Returns the next complete event payload, or a non-Event status.
    /// On @ref Status::Event, @p payload holds the joined `data` value.
    Status next_event(std::string& payload);

private:
    bool take_line(std::string& line);

    std::string buffer_;
    std::size_t cursor_ = 0;
    std::string data_; // Accumulated data across the current event's lines.
    bool has_data_ = false;
    bool eof_ = false;
};

} // namespace skailar::detail

#endif // SKAILAR_SSE_PARSER_HPP
