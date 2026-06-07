#include "sse_parser.hpp"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using skailar::detail::SseParser;

namespace {

// Feeds the whole input, then collects every event payload until the stream
// reports Done or NeedMore at EOF. Returns the payloads and whether Done was hit.
std::pair<std::vector<std::string>, bool> drain(const std::string& input) {
    SseParser parser;
    parser.feed(input.data(), input.size());
    parser.mark_eof();

    std::vector<std::string> events;
    bool done = false;
    for (;;) {
        std::string payload;
        const SseParser::Status status = parser.next_event(payload);
        if (status == SseParser::Status::Event) {
            events.push_back(payload);
        } else if (status == SseParser::Status::Done) {
            done = true;
            break;
        } else {
            break;
        }
    }
    return {events, done};
}

} // namespace

TEST(SseParser, ParsesLineFeedTerminator) {
    const auto [events, done] = drain("data: {\"a\":1}\n\ndata: [DONE]\n\n");
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events.at(0), "{\"a\":1}");
    EXPECT_TRUE(done);
}

TEST(SseParser, ParsesCarriageReturnLineFeedTerminator) {
    const auto [events, done] = drain("data: {\"a\":1}\r\n\r\ndata: [DONE]\r\n\r\n");
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events.at(0), "{\"a\":1}");
    EXPECT_TRUE(done);
}

TEST(SseParser, ParsesBareCarriageReturnTerminator) {
    const auto [events, done] = drain("data: {\"a\":1}\r\rdata: [DONE]\r\r");
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events.at(0), "{\"a\":1}");
    EXPECT_TRUE(done);
}

TEST(SseParser, SkipsCommentsAndBlankLines) {
    const auto [events, done] = drain(": this is a comment\n\ndata: {\"x\":true}\n\n");
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events.at(0), "{\"x\":true}");
    EXPECT_FALSE(done);
}

TEST(SseParser, HandlesDataWithoutSpaceAfterColon) {
    const auto [events, done] = drain("data:{\"y\":2}\n\n");
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events.at(0), "{\"y\":2}");
}

TEST(SseParser, JoinsMultipleDataLines) {
    const auto [events, done] = drain("data: line1\ndata: line2\n\n");
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events.at(0), "line1\nline2");
}

TEST(SseParser, DoneSentinelEndsStream) {
    const auto [events, done] = drain("data: [DONE]\n\n");
    EXPECT_TRUE(events.empty());
    EXPECT_TRUE(done);
}

TEST(SseParser, IncrementalFeedAssemblesSplitEvent) {
    SseParser parser;
    parser.feed("data: {\"par", 11);
    std::string payload;
    EXPECT_EQ(parser.next_event(payload), SseParser::Status::NeedMore);

    parser.feed("tial\":1}\n\n", 10);
    ASSERT_EQ(parser.next_event(payload), SseParser::Status::Event);
    EXPECT_EQ(payload, "{\"partial\":1}");
}

TEST(SseParser, SplitCarriageReturnLineFeedAcrossFeeds) {
    SseParser parser;
    parser.feed("data: x\r", 8);
    std::string payload;
    // A lone trailing \r may be the first half of \r\n; defer until more bytes.
    EXPECT_EQ(parser.next_event(payload), SseParser::Status::NeedMore);

    parser.feed("\n\n", 2);
    ASSERT_EQ(parser.next_event(payload), SseParser::Status::Event);
    EXPECT_EQ(payload, "x");
}

TEST(SseParser, EventWithoutTrailingBlankLineAtEof) {
    const auto [events, done] = drain("data: {\"z\":9}\n");
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events.at(0), "{\"z\":9}");
}
