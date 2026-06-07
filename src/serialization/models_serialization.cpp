#include <skailar/models.hpp>

#include "json_helpers.hpp"

#include <cstdint>

#include <nlohmann/json.hpp>

namespace skailar {

void to_json(json& j, const ModelCapabilities& c) {
    j = json {
        {"streaming", c.streaming},
        {"tool_calls", c.tool_calls},
        {"vision", c.vision},
        {"json_mode", c.json_mode},
    };
    detail::assign_optional(j, "reasoning", c.reasoning);
}

void from_json(const json& j, ModelCapabilities& c) {
    c.streaming = j.value("streaming", false);
    c.tool_calls = j.value("tool_calls", false);
    c.vision = j.value("vision", false);
    c.json_mode = j.value("json_mode", false);
    detail::read_optional(j, "reasoning", c.reasoning);
}

void to_json(json& j, const ModelPricing& p) {
    j = json {
        {"input_per_mtok", p.input_per_mtok},
        {"output_per_mtok", p.output_per_mtok},
        {"currency", p.currency},
    };
}

void from_json(const json& j, ModelPricing& p) {
    p.input_per_mtok = j.value("input_per_mtok", 0.0);
    p.output_per_mtok = j.value("output_per_mtok", 0.0);
    p.currency = detail::get_string(j, "currency");
}

void to_json(json& j, const ModelSummary& m) {
    j = json {
        {"id", m.id},
        {"object", m.object},
        {"created", m.created},
        {"owned_by", m.owned_by},
        {"display_name", m.display_name},
        {"context_window", m.context_window},
        {"max_output_tokens", m.max_output_tokens},
        {"capabilities", m.capabilities},
        {"pricing", m.pricing},
        {"status", m.status},
    };
}

void from_json(const json& j, ModelSummary& m) {
    m.id = detail::get_string(j, "id");
    m.object = detail::get_string(j, "object");
    m.created = j.value("created", std::int64_t {0});
    m.owned_by = detail::get_string(j, "owned_by");
    m.display_name = detail::get_string(j, "display_name");
    m.context_window = j.value("context_window", 0);
    m.max_output_tokens = j.value("max_output_tokens", 0);
    if (auto it = j.find("capabilities"); it != j.end() && it->is_object()) {
        it->get_to(m.capabilities);
    }
    if (auto it = j.find("pricing"); it != j.end() && it->is_object()) {
        it->get_to(m.pricing);
    }
    m.status = detail::get_string(j, "status");
}

void to_json(json& j, const Modalities& m) {
    j = json::object();
    detail::assign_optional(j, "input", m.input);
    detail::assign_optional(j, "output", m.output);
}

void from_json(const json& j, Modalities& m) {
    detail::read_optional(j, "input", m.input);
    detail::read_optional(j, "output", m.output);
}

void to_json(json& j, const Model& m) {
    j = json {
        {"id", m.id},
        {"object", m.object},
        {"created", m.created},
        {"owned_by", m.owned_by},
        {"display_name", m.display_name},
        {"context_window", m.context_window},
        {"max_output_tokens", m.max_output_tokens},
        {"capabilities", m.capabilities},
        {"pricing", m.pricing},
        {"status", m.status},
    };
    detail::assign_optional(j, "description", m.description);
    detail::assign_optional(j, "modalities", m.modalities);
    detail::assign_optional(j, "supported_parameters", m.supported_parameters);
    detail::assign_optional(j, "knowledge_cutoff", m.knowledge_cutoff);
    detail::assign_optional(j, "released_at", m.released_at);
    detail::assign_optional(j, "documentation_url", m.documentation_url);
    detail::assign_optional(j, "aliases", m.aliases);
}

void from_json(const json& j, Model& m) {
    m.id = detail::get_string(j, "id");
    m.object = detail::get_string(j, "object");
    m.created = j.value("created", std::int64_t {0});
    m.owned_by = detail::get_string(j, "owned_by");
    m.display_name = detail::get_string(j, "display_name");
    m.context_window = j.value("context_window", 0);
    m.max_output_tokens = j.value("max_output_tokens", 0);
    if (auto it = j.find("capabilities"); it != j.end() && it->is_object()) {
        it->get_to(m.capabilities);
    }
    if (auto it = j.find("pricing"); it != j.end() && it->is_object()) {
        it->get_to(m.pricing);
    }
    m.status = detail::get_string(j, "status");
    detail::read_optional(j, "description", m.description);
    detail::read_optional(j, "modalities", m.modalities);
    detail::read_optional(j, "supported_parameters", m.supported_parameters);
    detail::read_optional(j, "knowledge_cutoff", m.knowledge_cutoff);
    detail::read_optional(j, "released_at", m.released_at);
    detail::read_optional(j, "documentation_url", m.documentation_url);
    detail::read_optional(j, "aliases", m.aliases);
}

void to_json(json& j, const ModelList& l) {
    j = json {{"object", l.object}, {"data", l.data}};
}

void from_json(const json& j, ModelList& l) {
    l.object = detail::get_string(j, "object");
    if (auto it = j.find("data"); it != j.end()) {
        it->get_to(l.data);
    }
}

} // namespace skailar
