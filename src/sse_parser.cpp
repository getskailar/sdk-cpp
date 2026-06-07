#include "sse_parser.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace skailar::detail {

namespace {

constexpr std::string_view kDoneSentinel = "[DONE]";

} // namespace

void SseParser::feed(const char* data, std::size_t size) {
    buffer_.append(data, size);
}

// Extracts one line (without its terminator) starting at cursor_, accepting
// \n, \r\n, and \r. Returns false when no complete line is buffered, deferring a
// lone trailing \r unless at EOF (it may be the first half of a split \r\n).
bool SseParser::take_line(std::string& line) {
    for (std::size_t i = cursor_; i < buffer_.size(); ++i) {
        const char c = buffer_[i];
        if (c == '\n') {
            line.assign(buffer_, cursor_, i - cursor_);
            cursor_ = i + 1;
            return true;
        }
        if (c == '\r') {
            if (i + 1 < buffer_.size()) {
                const std::size_t end = i;
                const std::size_t next = (buffer_[i + 1] == '\n') ? i + 2 : i + 1;
                line.assign(buffer_, cursor_, end - cursor_);
                cursor_ = next;
                return true;
            }
            if (eof_) {
                line.assign(buffer_, cursor_, i - cursor_);
                cursor_ = i + 1;
                return true;
            }
            break; // Possible split \r\n; wait for more bytes.
        }
    }

    if (eof_ && cursor_ < buffer_.size()) {
        line.assign(buffer_, cursor_, buffer_.size() - cursor_);
        cursor_ = buffer_.size();
        return true;
    }

    // Compact consumed bytes so the buffer does not grow without bound.
    if (cursor_ > 0) {
        buffer_.erase(0, cursor_);
        cursor_ = 0;
    }
    return false;
}

SseParser::Status SseParser::next_event(std::string& payload) {
    std::string line;
    while (take_line(line)) {
        // A blank line dispatches the accumulated event.
        if (line.empty()) {
            if (has_data_) {
                payload = data_;
                data_.clear();
                has_data_ = false;
                if (std::string_view(payload) == kDoneSentinel) {
                    return Status::Done;
                }
                return Status::Event;
            }
            continue;
        }

        // Comment line.
        if (line[0] == ':') {
            continue;
        }

        std::string_view view(line);
        if (view.rfind("data:", 0) == 0) {
            std::string_view value = view.substr(5);
            if (!value.empty() && value.front() == ' ') {
                value.remove_prefix(1);
            }
            if (std::string_view(value) == kDoneSentinel) {
                data_.clear();
                has_data_ = false;
                payload.assign(kDoneSentinel);
                return Status::Done;
            }
            if (has_data_) {
                data_.push_back('\n');
            }
            data_.append(value);
            has_data_ = true;
        }
        // Other SSE fields (event:, id:, retry:) are ignored.
    }

    // At EOF an event may be pending without a trailing blank line.
    if (eof_ && has_data_) {
        payload = data_;
        data_.clear();
        has_data_ = false;
        if (std::string_view(payload) == kDoneSentinel) {
            return Status::Done;
        }
        return Status::Event;
    }

    return Status::NeedMore;
}

} // namespace skailar::detail
