#ifndef SKAILAR_TYPES_SHARED_HPP
#define SKAILAR_TYPES_SHARED_HPP

#include <skailar/json_fwd.hpp>

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace skailar {

/// Token accounting for a completion.
struct Usage {
    /// Number of tokens in the prompt.
    int prompt_tokens = 0;
    /// Number of generated tokens.
    int completion_tokens = 0;
    /// Sum of prompt and completion tokens.
    int total_tokens = 0;
};

void to_json(json& j, const Usage& u);
void from_json(const json& j, Usage& u);

/// Function name and arguments within a @ref ToolCall.
struct FunctionCall {
    /// Called function name.
    std::string name;
    /// JSON-encoded arguments string, as produced by the model.
    std::string arguments;
};

void to_json(json& j, const FunctionCall& f);
void from_json(const json& j, FunctionCall& f);

/// A function call requested by the model.
struct ToolCall {
    /// Identifies this tool call, for pairing with a tool-result message.
    std::string id;
    /// Tool type; always "function".
    std::string type = "function";
    /// Called function name and arguments.
    FunctionCall function;
};

void to_json(json& j, const ToolCall& t);
void from_json(const json& j, ToolCall& t);

/// Describes a callable function in a @ref Tool.
struct FunctionDefinition {
    /// Function name.
    std::string name;
    /// Optional human-readable description.
    std::optional<std::string> description;
    /// Optional JSON Schema object describing the arguments.
    std::optional<json> parameters;
};

void to_json(json& j, const FunctionDefinition& f);
void from_json(const json& j, FunctionDefinition& f);

/// A function-calling tool definition, OpenAI-compatible.
struct Tool {
    /// Tool type; always "function".
    std::string type = "function";
    /// The callable function.
    FunctionDefinition function;
};

void to_json(json& j, const Tool& t);
void from_json(const json& j, Tool& t);

/// Builds a @ref Tool of type "function" from a name, description, and JSON
/// Schema parameter object.
Tool function_tool(std::string name, std::string description, json parameters);

/// Forces the model to call a specific named function. Used inside @ref ToolChoice.
struct NamedToolChoice {
    /// Name of the function the model must call.
    std::string name;
};

/// Constrains how the model may call tools.
///
/// Holds either a mode string ("auto", "none", "required") or a
/// @ref NamedToolChoice. On the wire it marshals as a bare string or a
/// named-function object. Construct with the static factories.
class ToolChoice {
public:
    ToolChoice() = default;

    /// Lets the model decide whether to call a tool.
    static ToolChoice automatic();
    /// Forbids tool calls.
    static ToolChoice none();
    /// Forces the model to call at least one tool.
    static ToolChoice required();
    /// Forces the model to call the named function.
    static ToolChoice named(std::string name);

    /// Reports whether this is a mode string (as opposed to a named choice).
    bool is_mode() const noexcept { return std::holds_alternative<std::string>(value_); }
    /// Reports whether this forces a specific named function.
    bool is_named() const noexcept { return std::holds_alternative<NamedToolChoice>(value_); }

    /// Returns the mode string. Throws std::bad_variant_access if @ref is_named.
    const std::string& mode() const { return std::get<std::string>(value_); }
    /// Returns the named choice. Throws std::bad_variant_access if @ref is_mode.
    const NamedToolChoice& name() const { return std::get<NamedToolChoice>(value_); }

private:
    explicit ToolChoice(std::variant<std::string, NamedToolChoice> v) : value_(std::move(v)) { }

    std::variant<std::string, NamedToolChoice> value_ {std::string("auto")};
};

void to_json(json& j, const ToolChoice& t);
void from_json(const json& j, ToolChoice& t);

} // namespace skailar

#endif // SKAILAR_TYPES_SHARED_HPP
