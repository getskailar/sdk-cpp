#include <skailar/images.hpp>

#include "json_helpers.hpp"

#include <cstdint>

#include <nlohmann/json.hpp>

namespace skailar {

void to_json(json& j, const ImageGenerationRequest& r) {
    j = json::object();
    j["model"] = r.model;
    j["prompt"] = r.prompt;
    detail::assign_optional(j, "n", r.n);
    detail::assign_optional(j, "size", r.size);
    detail::assign_optional(j, "quality", r.quality);
    detail::assign_optional(j, "background", r.background);
}

void from_json(const json& j, ImageGenerationRequest& r) {
    r.model = detail::get_string(j, "model");
    r.prompt = detail::get_string(j, "prompt");
    detail::read_optional(j, "n", r.n);
    detail::read_optional(j, "size", r.size);
    detail::read_optional(j, "quality", r.quality);
    detail::read_optional(j, "background", r.background);
}

void to_json(json& j, const GeneratedImage& g) {
    j = json::object();
    detail::assign_optional(j, "url", g.url);
    detail::assign_optional(j, "b64_json", g.b64_json);
    detail::assign_optional(j, "revised_prompt", g.revised_prompt);
}

void from_json(const json& j, GeneratedImage& g) {
    detail::read_optional(j, "url", g.url);
    detail::read_optional(j, "b64_json", g.b64_json);
    detail::read_optional(j, "revised_prompt", g.revised_prompt);
}

void to_json(json& j, const ImageGenerationResponse& r) {
    j = json {{"created", r.created}, {"data", r.data}};
}

void from_json(const json& j, ImageGenerationResponse& r) {
    r.created = j.value("created", std::int64_t {0});
    if (auto it = j.find("data"); it != j.end()) {
        it->get_to(r.data);
    }
}

} // namespace skailar
