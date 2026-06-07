#include <skailar/chat.hpp>

#include "json_helpers.hpp"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace skailar {

namespace {

Role role_from_string(const std::string& s) {
    if (s == "system") {
        return Role::System;
    }
    if (s == "assistant") {
        return Role::Assistant;
    }
    if (s == "tool") {
        return Role::Tool;
    }
    return Role::User;
}

std::optional<ReasoningEffort> reasoning_from_string(const std::string& s) {
    if (s == "low") {
        return ReasoningEffort::Low;
    }
    if (s == "medium") {
        return ReasoningEffort::Medium;
    }
    if (s == "high") {
        return ReasoningEffort::High;
    }
    return std::nullopt;
}

FinishReason finish_from_string(const std::string& s) {
    if (s == "length") {
        return FinishReason::Length;
    }
    if (s == "tool_calls") {
        return FinishReason::ToolCalls;
    }
    if (s == "content_filter") {
        return FinishReason::ContentFilter;
    }
    return FinishReason::Stop;
}

ContentPartType content_part_type_from_string(const std::string& s) {
    if (s == "image_url") {
        return ContentPartType::ImageUrl;
    }
    return ContentPartType::Text;
}

} // namespace

const char* to_string(Role role) noexcept {
    switch (role) {
    case Role::System:
        return "system";
    case Role::User:
        return "user";
    case Role::Assistant:
        return "assistant";
    case Role::Tool:
        return "tool";
    }
    return "user";
}

const char* to_string(ReasoningEffort effort) noexcept {
    switch (effort) {
    case ReasoningEffort::Low:
        return "low";
    case ReasoningEffort::Medium:
        return "medium";
    case ReasoningEffort::High:
        return "high";
    }
    return "medium";
}

const char* to_string(FinishReason reason) noexcept {
    switch (reason) {
    case FinishReason::Stop:
        return "stop";
    case FinishReason::Length:
        return "length";
    case FinishReason::ToolCalls:
        return "tool_calls";
    case FinishReason::ContentFilter:
        return "content_filter";
    }
    return "stop";
}

void to_json(json& j, const ImageUrl& i) {
    j = json::object();
    j["url"] = i.url;
    detail::assign_optional(j, "detail", i.detail);
}

void from_json(const json& j, ImageUrl& i) {
    i.url = detail::get_string(j, "url");
    detail::read_optional(j, "detail", i.detail);
}

ContentPart text_part(std::string text) {
    ContentPart part;
    part.type = ContentPartType::Text;
    part.text = std::move(text);
    return part;
}

ContentPart image_part(std::string url) {
    ContentPart part;
    part.type = ContentPartType::ImageUrl;
    part.image_url = ImageUrl {std::move(url), std::nullopt};
    return part;
}

void to_json(json& j, const ContentPart& p) {
    j = json::object();
    if (p.type == ContentPartType::ImageUrl) {
        j["type"] = "image_url";
        if (p.image_url.has_value()) {
            j["image_url"] = *p.image_url;
        }
    } else {
        j["type"] = "text";
        j["text"] = p.text.value_or("");
    }
}

void from_json(const json& j, ContentPart& p) {
    p.type = content_part_type_from_string(detail::get_string(j, "type"));
    detail::read_optional(j, "text", p.text);
    detail::read_optional(j, "image_url", p.image_url);
}

MessageContent MessageContent::text(std::string s) {
    MessageContent c;
    c.value_ = std::move(s);
    return c;
}

MessageContent MessageContent::parts(std::vector<ContentPart> p) {
    MessageContent c;
    c.value_ = std::move(p);
    return c;
}

std::string MessageContent::text() const {
    if (is_text()) {
        return as_text();
    }
    std::string out;
    for (const auto& part : as_parts()) {
        if (part.type == ContentPartType::Text && part.text.has_value()) {
            out += *part.text;
        }
    }
    return out;
}

void to_json(json& j, const MessageContent& c) {
    if (c.is_parts()) {
        j = c.as_parts();
    } else {
        j = c.as_text();
    }
}

void from_json(const json& j, MessageContent& c) {
    if (j.is_array()) {
        c = MessageContent::parts(j.get<std::vector<ContentPart>>());
    } else if (j.is_string()) {
        c = MessageContent::text(j.get<std::string>());
    } else {
        c = MessageContent::text("");
    }
}

void to_json(json& j, const ChatMessage& m) {
    j = json::object();
    j["role"] = to_string(m.role);
    if (m.content.has_value()) {
        j["content"] = *m.content;
    }
    detail::assign_optional(j, "tool_calls", m.tool_calls);
    detail::assign_optional(j, "tool_call_id", m.tool_call_id);
}

void from_json(const json& j, ChatMessage& m) {
    m.role = role_from_string(detail::get_string(j, "role"));
    if (auto it = j.find("content"); it != j.end() && !it->is_null()) {
        m.content = it->get<MessageContent>();
    } else {
        m.content.reset();
    }
    detail::read_optional(j, "tool_calls", m.tool_calls);
    detail::read_optional(j, "tool_call_id", m.tool_call_id);
}

ChatMessage system_message(std::string text) {
    ChatMessage m;
    m.role = Role::System;
    m.content = MessageContent::text(std::move(text));
    return m;
}

ChatMessage user_message(std::string text) {
    ChatMessage m;
    m.role = Role::User;
    m.content = MessageContent::text(std::move(text));
    return m;
}

ChatMessage assistant_message(std::string text) {
    ChatMessage m;
    m.role = Role::Assistant;
    m.content = MessageContent::text(std::move(text));
    return m;
}

ChatMessage tool_message(std::string tool_call_id, std::string text) {
    ChatMessage m;
    m.role = Role::Tool;
    m.content = MessageContent::text(std::move(text));
    m.tool_call_id = std::move(tool_call_id);
    return m;
}

StopSequence StopSequence::one(std::string s) {
    StopSequence stop;
    stop.value_ = std::move(s);
    return stop;
}

StopSequence StopSequence::many(std::vector<std::string> s) {
    StopSequence stop;
    stop.value_ = std::move(s);
    return stop;
}

void to_json(json& j, const StopSequence& s) {
    if (s.is_many()) {
        j = s.as_many();
    } else {
        j = s.as_one();
    }
}

void from_json(const json& j, StopSequence& s) {
    if (j.is_array()) {
        s = StopSequence::many(j.get<std::vector<std::string>>());
    } else {
        s = StopSequence::one(j.is_string() ? j.get<std::string>() : std::string());
    }
}

void to_json(json& j, const ChatCompletionRequest& r) {
    j = json::object();
    j["model"] = r.model;
    j["messages"] = r.messages;
    detail::assign_optional(j, "stream", r.stream);
    detail::assign_optional(j, "max_tokens", r.max_tokens);
    detail::assign_optional(j, "temperature", r.temperature);
    detail::assign_optional(j, "top_p", r.top_p);
    if (r.reasoning_effort.has_value()) {
        j["reasoning_effort"] = to_string(*r.reasoning_effort);
    }
    detail::assign_optional(j, "tools", r.tools);
    detail::assign_optional(j, "tool_choice", r.tool_choice);
    detail::assign_optional(j, "response_format", r.response_format);
    detail::assign_optional(j, "n", r.n);
    detail::assign_optional(j, "presence_penalty", r.presence_penalty);
    detail::assign_optional(j, "frequency_penalty", r.frequency_penalty);
    detail::assign_optional(j, "logit_bias", r.logit_bias);
    detail::assign_optional(j, "user", r.user);
    detail::assign_optional(j, "seed", r.seed);
    detail::assign_optional(j, "stop", r.stop);
}

void from_json(const json& j, ChatCompletionRequest& r) {
    r.model = detail::get_string(j, "model");
    if (auto it = j.find("messages"); it != j.end()) {
        it->get_to(r.messages);
    }
    detail::read_optional(j, "stream", r.stream);
    detail::read_optional(j, "max_tokens", r.max_tokens);
    detail::read_optional(j, "temperature", r.temperature);
    detail::read_optional(j, "top_p", r.top_p);
    if (auto it = j.find("reasoning_effort"); it != j.end() && it->is_string()) {
        r.reasoning_effort = reasoning_from_string(it->get<std::string>());
    }
    detail::read_optional(j, "tools", r.tools);
    detail::read_optional(j, "tool_choice", r.tool_choice);
    detail::read_optional(j, "response_format", r.response_format);
    detail::read_optional(j, "n", r.n);
    detail::read_optional(j, "presence_penalty", r.presence_penalty);
    detail::read_optional(j, "frequency_penalty", r.frequency_penalty);
    detail::read_optional(j, "logit_bias", r.logit_bias);
    detail::read_optional(j, "user", r.user);
    detail::read_optional(j, "seed", r.seed);
    detail::read_optional(j, "stop", r.stop);
}

void to_json(json& j, const ResponseMessage& m) {
    j = json::object();
    j["role"] = to_string(m.role);
    j["content"] = m.content;
    detail::assign_optional(j, "reasoning_content", m.reasoning_content);
    detail::assign_optional(j, "tool_calls", m.tool_calls);
}

void from_json(const json& j, ResponseMessage& m) {
    m.role = role_from_string(detail::get_string(j, "role"));
    if (auto it = j.find("content"); it != j.end() && !it->is_null()) {
        m.content = it->get<MessageContent>();
    } else {
        m.content = MessageContent::text("");
    }
    detail::read_optional(j, "reasoning_content", m.reasoning_content);
    detail::read_optional(j, "tool_calls", m.tool_calls);
}

void to_json(json& j, const Choice& c) {
    j = json {
        {"index", c.index},
        {"message", c.message},
        {"finish_reason", to_string(c.finish_reason)},
    };
}

void from_json(const json& j, Choice& c) {
    j.at("index").get_to(c.index);
    j.at("message").get_to(c.message);
    c.finish_reason = finish_from_string(detail::get_string(j, "finish_reason"));
}

void to_json(json& j, const ChatCompletionResponse& r) {
    j = json {
        {"id", r.id},       {"object", r.object},   {"created", r.created},
        {"model", r.model}, {"choices", r.choices}, {"usage", r.usage},
    };
}

void from_json(const json& j, ChatCompletionResponse& r) {
    r.id = detail::get_string(j, "id");
    r.object = detail::get_string(j, "object");
    r.created = j.value("created", std::int64_t {0});
    r.model = detail::get_string(j, "model");
    if (auto it = j.find("choices"); it != j.end()) {
        it->get_to(r.choices);
    }
    if (auto it = j.find("usage"); it != j.end() && it->is_object()) {
        it->get_to(r.usage);
    }
}

void to_json(json& j, const FunctionCallDelta& f) {
    j = json::object();
    detail::assign_optional(j, "name", f.name);
    detail::assign_optional(j, "arguments", f.arguments);
}

void from_json(const json& j, FunctionCallDelta& f) {
    detail::read_optional(j, "name", f.name);
    detail::read_optional(j, "arguments", f.arguments);
}

void to_json(json& j, const ToolCallDelta& t) {
    j = json::object();
    j["index"] = t.index;
    detail::assign_optional(j, "id", t.id);
    detail::assign_optional(j, "type", t.type);
    detail::assign_optional(j, "function", t.function);
}

void from_json(const json& j, ToolCallDelta& t) {
    t.index = j.value("index", 0);
    detail::read_optional(j, "id", t.id);
    detail::read_optional(j, "type", t.type);
    detail::read_optional(j, "function", t.function);
}

void to_json(json& j, const Delta& d) {
    j = json::object();
    if (d.role.has_value()) {
        j["role"] = to_string(*d.role);
    }
    detail::assign_optional(j, "content", d.content);
    detail::assign_optional(j, "reasoning_content", d.reasoning_content);
    detail::assign_optional(j, "tool_calls", d.tool_calls);
}

void from_json(const json& j, Delta& d) {
    if (auto it = j.find("role"); it != j.end() && it->is_string()) {
        d.role = role_from_string(it->get<std::string>());
    } else {
        d.role.reset();
    }
    detail::read_optional(j, "content", d.content);
    detail::read_optional(j, "reasoning_content", d.reasoning_content);
    detail::read_optional(j, "tool_calls", d.tool_calls);
}

void to_json(json& j, const ChunkChoice& c) {
    j = json::object();
    j["index"] = c.index;
    j["delta"] = c.delta;
    if (c.finish_reason.has_value()) {
        j["finish_reason"] = to_string(*c.finish_reason);
    }
}

void from_json(const json& j, ChunkChoice& c) {
    c.index = j.value("index", 0);
    if (auto it = j.find("delta"); it != j.end()) {
        it->get_to(c.delta);
    }
    if (auto it = j.find("finish_reason"); it != j.end() && it->is_string()) {
        c.finish_reason = finish_from_string(it->get<std::string>());
    } else {
        c.finish_reason.reset();
    }
}

std::optional<std::string> ChatCompletionChunk::content_delta() const {
    if (choices.empty()) {
        return std::nullopt;
    }
    const auto& delta = choices.front().delta;
    if (delta.content.has_value() && !delta.content->empty()) {
        return delta.content;
    }
    return std::nullopt;
}

void to_json(json& j, const ChatCompletionChunk& c) {
    j = json {
        {"id", c.id},       {"object", c.object},   {"created", c.created},
        {"model", c.model}, {"choices", c.choices},
    };
    detail::assign_optional(j, "usage", c.usage);
}

void from_json(const json& j, ChatCompletionChunk& c) {
    c.id = detail::get_string(j, "id");
    c.object = detail::get_string(j, "object");
    c.created = j.value("created", std::int64_t {0});
    c.model = detail::get_string(j, "model");
    if (auto it = j.find("choices"); it != j.end()) {
        it->get_to(c.choices);
    }
    if (auto it = j.find("usage"); it != j.end() && it->is_object()) {
        c.usage = it->get<Usage>();
    } else {
        c.usage.reset();
    }
}

} // namespace skailar
