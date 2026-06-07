#ifndef SKAILAR_CHAT_HPP
#define SKAILAR_CHAT_HPP

#include <skailar/json_fwd.hpp>
#include <skailar/types_shared.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace skailar {

/// The author role of a @ref ChatMessage.
enum class Role : std::uint8_t {
    /// System or developer instructions.
    System,
    /// End-user input.
    User,
    /// Model output.
    Assistant,
    /// The result of a tool call, paired with a tool-call id.
    Tool,
};

/// Returns the wire string for @p role ("system", "user", "assistant", "tool").
const char* to_string(Role role) noexcept;

/// The reasoning budget for reasoning-capable models.
enum class ReasoningEffort : std::uint8_t {
    /// Minimal reasoning.
    Low,
    /// Balanced reasoning.
    Medium,
    /// Maximum reasoning.
    High,
};

/// Returns the wire string for @p effort ("low", "medium", "high").
const char* to_string(ReasoningEffort effort) noexcept;

/// Why a completion stopped generating.
enum class FinishReason : std::uint8_t {
    /// A natural stop or a stop sequence was hit.
    Stop,
    /// The token limit was reached.
    Length,
    /// The model emitted tool calls.
    ToolCalls,
    /// The output was filtered.
    ContentFilter,
};

/// Returns the wire string for @p reason.
const char* to_string(FinishReason reason) noexcept;

/// An image reference within a @ref ContentPart.
struct ImageUrl {
    /// A `data:` URI or an HTTPS URL (for example, from @ref ImageUploadsResource).
    std::string url;
    /// Optional detail hint: "low", "high", or "auto".
    std::optional<std::string> detail;
};

void to_json(json& j, const ImageUrl& i);
void from_json(const json& j, ImageUrl& i);

/// Discriminates a @ref ContentPart.
enum class ContentPartType : std::uint8_t {
    /// A run of text.
    Text,
    /// An image reference.
    ImageUrl,
};

/// One part of a multimodal message.
struct ContentPart {
    /// Discriminates the part.
    ContentPartType type = ContentPartType::Text;
    /// Set when @ref type is @ref ContentPartType::Text.
    std::optional<std::string> text;
    /// Set when @ref type is @ref ContentPartType::ImageUrl.
    std::optional<skailar::ImageUrl> image_url;
};

void to_json(json& j, const ContentPart& p);
void from_json(const json& j, ContentPart& p);

/// Builds a text @ref ContentPart.
ContentPart text_part(std::string text);

/// Builds an image @ref ContentPart from a `data:` URI or HTTPS URL.
ContentPart image_part(std::string url);

/// The content of a @ref ChatMessage: either a single text string or an ordered
/// list of multimodal parts.
///
/// On the wire it is a string or an array of @ref ContentPart. Construct it with
/// @ref text or @ref parts, and read the plain text with @ref text() const.
class MessageContent {
public:
    MessageContent() = default;

    /// Builds content holding a single text string.
    static MessageContent text(std::string s);
    /// Builds content holding multimodal parts.
    static MessageContent parts(std::vector<ContentPart> p);

    /// Reports whether the content is a single text string.
    bool is_text() const noexcept { return std::holds_alternative<std::string>(value_); }
    /// Reports whether the content is a list of multimodal parts.
    bool is_parts() const noexcept {
        return std::holds_alternative<std::vector<ContentPart>>(value_);
    }

    /// Returns the text string. Throws std::bad_variant_access if @ref is_parts.
    const std::string& as_text() const { return std::get<std::string>(value_); }
    /// Returns the parts. Throws std::bad_variant_access if @ref is_text.
    const std::vector<ContentPart>& as_parts() const {
        return std::get<std::vector<ContentPart>>(value_);
    }

    /// Returns the plain text. For multimodal content it concatenates the text
    /// of every text part; image parts contribute nothing.
    std::string text() const;

private:
    std::variant<std::string, std::vector<ContentPart>> value_ {std::string()};
};

/// Builds @ref MessageContent holding a single text string.
inline MessageContent text_content(std::string s) {
    return MessageContent::text(std::move(s));
}

/// A single message in a chat conversation.
struct ChatMessage {
    /// The author role.
    Role role = Role::User;
    /// The message content. Omitted on the wire when an assistant message only
    /// carries tool calls; leave empty in that case.
    std::optional<MessageContent> content;
    /// Tool calls requested by an assistant message.
    std::optional<std::vector<ToolCall>> tool_calls;
    /// Identifies the tool call this message responds to; required when @ref role
    /// is @ref Role::Tool.
    std::optional<std::string> tool_call_id;
};

void to_json(json& j, const ChatMessage& m);
void from_json(const json& j, ChatMessage& m);

/// Builds a system message from text.
ChatMessage system_message(std::string text);
/// Builds a user message from text.
ChatMessage user_message(std::string text);
/// Builds an assistant message from text.
ChatMessage assistant_message(std::string text);
/// Builds a tool-result message from a tool-call id and text.
ChatMessage tool_message(std::string tool_call_id, std::string text);

/// A stop condition: a single string or several.
///
/// On the wire it marshals as a bare string or an array of strings.
class StopSequence {
public:
    StopSequence() = default;

    /// Builds a single-sequence stop condition.
    static StopSequence one(std::string s);
    /// Builds a multi-sequence stop condition.
    static StopSequence many(std::vector<std::string> s);

    /// Reports whether this is a single stop string.
    bool is_one() const noexcept { return std::holds_alternative<std::string>(value_); }
    /// Reports whether this is several stop strings.
    bool is_many() const noexcept {
        return std::holds_alternative<std::vector<std::string>>(value_);
    }

    /// Returns the single stop string. Throws std::bad_variant_access otherwise.
    const std::string& as_one() const { return std::get<std::string>(value_); }
    /// Returns the stop strings. Throws std::bad_variant_access otherwise.
    const std::vector<std::string>& as_many() const {
        return std::get<std::vector<std::string>>(value_);
    }

private:
    std::variant<std::string, std::vector<std::string>> value_ {std::string()};
};

void to_json(json& j, const StopSequence& s);
void from_json(const json& j, StopSequence& s);

/// A request to create a chat completion. Optional fields are omitted from the
/// wire body when unset.
struct ChatCompletionRequest {
    /// Model identifier or alias. See the @ref models namespace and
    /// @ref ModelsResource::list.
    std::string model;
    /// The conversation so far.
    std::vector<ChatMessage> messages;
    /// Requests an SSE stream instead of a single response.
    /// @ref ChatCompletionsResource::create_stream sets this automatically.
    std::optional<bool> stream;
    /// Caps the number of generated tokens.
    std::optional<int> max_tokens;
    /// Sampling temperature in [0, 2].
    std::optional<float> temperature;
    /// Nucleus sampling probability in [0, 1].
    std::optional<float> top_p;
    /// Reasoning budget for reasoning-capable models.
    std::optional<ReasoningEffort> reasoning_effort;
    /// Tool definitions the model may call.
    std::optional<std::vector<Tool>> tools;
    /// Constrains tool calling.
    std::optional<ToolChoice> tool_choice;
    /// OpenAI-compatible response format object, passed through unchanged.
    std::optional<json> response_format;
    /// Number of completions to generate.
    std::optional<int> n;
    /// Penalizes token presence.
    std::optional<float> presence_penalty;
    /// Penalizes token frequency.
    std::optional<float> frequency_penalty;
    /// Per-token logit bias map, passed through unchanged.
    std::optional<json> logit_bias;
    /// End-user identifier for abuse monitoring.
    std::optional<std::string> user;
    /// Requests best-effort determinism.
    std::optional<std::int64_t> seed;
    /// One or more stop sequences.
    std::optional<StopSequence> stop;
};

void to_json(json& j, const ChatCompletionRequest& r);
void from_json(const json& j, ChatCompletionRequest& r);

/// The assistant message inside a @ref Choice.
struct ResponseMessage {
    /// The author role; always @ref Role::Assistant.
    Role role = Role::Assistant;
    /// The generated text.
    MessageContent content;
    /// The reasoning trace, for reasoning-capable models.
    std::optional<std::string> reasoning_content;
    /// Tool calls requested by the model.
    std::optional<std::vector<ToolCall>> tool_calls;
};

void to_json(json& j, const ResponseMessage& m);
void from_json(const json& j, ResponseMessage& m);

/// One choice within a @ref ChatCompletionResponse.
struct Choice {
    /// Position of this choice in the list.
    int index = 0;
    /// The generated message.
    ResponseMessage message;
    /// Why generation stopped.
    FinishReason finish_reason = FinishReason::Stop;
};

void to_json(json& j, const Choice& c);
void from_json(const json& j, Choice& c);

/// A non-streamed chat completion.
struct ChatCompletionResponse {
    /// Unique completion identifier.
    std::string id;
    /// Object type; always "chat.completion".
    std::string object;
    /// Unix epoch seconds at creation.
    std::int64_t created = 0;
    /// The model that produced the completion.
    std::string model;
    /// One entry per generated choice.
    std::vector<Choice> choices;
    /// Token accounting.
    Usage usage;
};

void to_json(json& j, const ChatCompletionResponse& r);
void from_json(const json& j, ChatCompletionResponse& r);

/// An incremental function name/arguments within a @ref ToolCallDelta.
struct FunctionCallDelta {
    /// The function name, present on the first fragment.
    std::optional<std::string> name;
    /// An argument-string fragment to be concatenated.
    std::optional<std::string> arguments;
};

void to_json(json& j, const FunctionCallDelta& f);
void from_json(const json& j, FunctionCallDelta& f);

/// An incremental tool-call fragment within a @ref Delta.
struct ToolCallDelta {
    /// Index of the tool call being assembled.
    int index = 0;
    /// The tool-call id, present on the first fragment.
    std::optional<std::string> id;
    /// The discriminator, present on the first fragment.
    std::optional<std::string> type;
    /// Function name and argument fragments.
    std::optional<FunctionCallDelta> function;
};

void to_json(json& j, const ToolCallDelta& t);
void from_json(const json& j, ToolCallDelta& t);

/// The incremental payload of a @ref ChunkChoice.
struct Delta {
    /// The author role, present on the first delta.
    std::optional<Role> role;
    /// A text fragment.
    std::optional<std::string> content;
    /// A reasoning-trace fragment.
    std::optional<std::string> reasoning_content;
    /// Incremental tool-call fragments.
    std::optional<std::vector<ToolCallDelta>> tool_calls;
};

void to_json(json& j, const Delta& d);
void from_json(const json& j, Delta& d);

/// One choice within a @ref ChatCompletionChunk.
struct ChunkChoice {
    /// Position of this choice in the list.
    int index = 0;
    /// The incremental payload for this choice.
    Delta delta;
    /// Why generation stopped, on the final chunk for this choice.
    std::optional<FinishReason> finish_reason;
};

void to_json(json& j, const ChunkChoice& c);
void from_json(const json& j, ChunkChoice& c);

/// One event in a streamed completion.
struct ChatCompletionChunk {
    /// The completion identifier, stable across the stream.
    std::string id;
    /// Object type; always "chat.completion.chunk".
    std::string object;
    /// Unix epoch seconds at creation.
    std::int64_t created = 0;
    /// The model producing the stream.
    std::string model;
    /// The incremental choices.
    std::vector<ChunkChoice> choices;
    /// Cumulative token accounting, present on the final chunk(s).
    std::optional<Usage> usage;

    /// Returns the text fragment of the first choice, or std::nullopt when this
    /// chunk carries no text delta. A convenience for the single-choice loop.
    std::optional<std::string> content_delta() const;
};

void to_json(json& j, const ChatCompletionChunk& c);
void from_json(const json& j, ChatCompletionChunk& c);

} // namespace skailar

#endif // SKAILAR_CHAT_HPP
