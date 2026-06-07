#include <skailar/types_shared.hpp>

#include "json_helpers.hpp"

#include <string>
#include <utility>

#include <nlohmann/json.hpp>

namespace skailar {

void to_json(json& j, const Usage& u) {
    j = json {
        {"prompt_tokens", u.prompt_tokens},
        {"completion_tokens", u.completion_tokens},
        {"total_tokens", u.total_tokens},
    };
}

void from_json(const json& j, Usage& u) {
    j.at("prompt_tokens").get_to(u.prompt_tokens);
    j.at("completion_tokens").get_to(u.completion_tokens);
    j.at("total_tokens").get_to(u.total_tokens);
}

void to_json(json& j, const FunctionCall& f) {
    j = json {{"name", f.name}, {"arguments", f.arguments}};
}

void from_json(const json& j, FunctionCall& f) {
    f.name = detail::get_string(j, "name");
    f.arguments = detail::get_string(j, "arguments");
}

void to_json(json& j, const ToolCall& t) {
    j = json {{"id", t.id}, {"type", t.type}, {"function", t.function}};
}

void from_json(const json& j, ToolCall& t) {
    t.id = detail::get_string(j, "id");
    t.type = detail::get_string(j, "type");
    if (auto it = j.find("function"); it != j.end()) {
        it->get_to(t.function);
    }
}

void to_json(json& j, const FunctionDefinition& f) {
    j = json::object();
    j["name"] = f.name;
    detail::assign_optional(j, "description", f.description);
    detail::assign_optional(j, "parameters", f.parameters);
}

void from_json(const json& j, FunctionDefinition& f) {
    f.name = detail::get_string(j, "name");
    detail::read_optional(j, "description", f.description);
    detail::read_optional(j, "parameters", f.parameters);
}

void to_json(json& j, const Tool& t) {
    j = json {{"type", t.type}, {"function", t.function}};
}

void from_json(const json& j, Tool& t) {
    t.type = detail::get_string(j, "type");
    if (auto it = j.find("function"); it != j.end()) {
        it->get_to(t.function);
    }
}

Tool function_tool(std::string name, std::string description, json parameters) {
    Tool tool;
    tool.type = "function";
    tool.function.name = std::move(name);
    tool.function.description = std::move(description);
    tool.function.parameters = std::move(parameters);
    return tool;
}

ToolChoice ToolChoice::automatic() {
    return ToolChoice(std::string("auto"));
}
ToolChoice ToolChoice::none() {
    return ToolChoice(std::string("none"));
}
ToolChoice ToolChoice::required() {
    return ToolChoice(std::string("required"));
}
ToolChoice ToolChoice::named(std::string name) {
    return ToolChoice(NamedToolChoice {std::move(name)});
}

void to_json(json& j, const ToolChoice& t) {
    if (t.is_named()) {
        j = json {
            {"type", "function"},
            {"function", json {{"name", t.name().name}}},
        };
    } else {
        j = t.mode();
    }
}

void from_json(const json& j, ToolChoice& t) {
    if (j.is_string()) {
        const std::string mode = j.get<std::string>();
        if (mode == "none") {
            t = ToolChoice::none();
        } else if (mode == "required") {
            t = ToolChoice::required();
        } else {
            t = ToolChoice::automatic();
        }
        return;
    }
    std::string name;
    if (auto fn = j.find("function"); fn != j.end() && fn->is_object()) {
        name = detail::get_string(*fn, "name");
    }
    t = ToolChoice::named(name);
}

} // namespace skailar
