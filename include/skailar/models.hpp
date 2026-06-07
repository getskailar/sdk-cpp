#ifndef SKAILAR_MODELS_HPP
#define SKAILAR_MODELS_HPP

#include <skailar/json_fwd.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace skailar {

/// Which features a model supports.
struct ModelCapabilities {
    /// Whether the model supports SSE streaming.
    bool streaming = false;
    /// Whether the model supports function calling.
    bool tool_calls = false;
    /// Whether the model accepts image input.
    bool vision = false;
    /// Whether the model supports JSON-constrained output.
    bool json_mode = false;
    /// Whether the model exposes a reasoning trace; unset when unknown.
    std::optional<bool> reasoning;
};

void to_json(json& j, const ModelCapabilities& c);
void from_json(const json& j, ModelCapabilities& c);

/// Per-million-token pricing for a model.
struct ModelPricing {
    /// Price per million input tokens.
    double input_per_mtok = 0.0;
    /// Price per million output tokens.
    double output_per_mtok = 0.0;
    /// ISO 4217 currency code, for example "USD".
    std::string currency;
};

void to_json(json& j, const ModelPricing& p);
void from_json(const json& j, ModelPricing& p);

/// The catalog entry for a model, as returned by @ref ModelsResource::list.
struct ModelSummary {
    /// Model identifier.
    std::string id;
    /// Object type; always "model".
    std::string object;
    /// Unix epoch seconds at registration.
    std::int64_t created = 0;
    /// Provider: "skailar", "anthropic", "openai", "google", "deepseek", "xai".
    std::string owned_by;
    /// Human-readable model name.
    std::string display_name;
    /// Maximum context length in tokens.
    int context_window = 0;
    /// Maximum number of tokens the model can generate.
    int max_output_tokens = 0;
    /// Supported features.
    ModelCapabilities capabilities;
    /// Per-million-token pricing.
    ModelPricing pricing;
    /// Lifecycle state: "active", "preview", or "deprecated".
    std::string status;
};

void to_json(json& j, const ModelSummary& m);
void from_json(const json& j, ModelSummary& m);

/// The input and output modalities of a model.
struct Modalities {
    /// Accepted input modalities, for example "text", "image".
    std::optional<std::vector<std::string>> input;
    /// Produced output modalities.
    std::optional<std::vector<std::string>> output;
};

void to_json(json& j, const Modalities& m);
void from_json(const json& j, Modalities& m);

/// The full detail card for a model, as returned by @ref ModelsResource::retrieve.
/// Includes every @ref ModelSummary field plus the detail fields below.
struct Model {
    /// Model identifier.
    std::string id;
    /// Object type; always "model".
    std::string object;
    /// Unix epoch seconds at registration.
    std::int64_t created = 0;
    /// Provider that owns the model.
    std::string owned_by;
    /// Human-readable model name.
    std::string display_name;
    /// Maximum context length in tokens.
    int context_window = 0;
    /// Maximum number of tokens the model can generate.
    int max_output_tokens = 0;
    /// Supported features.
    ModelCapabilities capabilities;
    /// Per-million-token pricing.
    ModelPricing pricing;
    /// Lifecycle state: "active", "preview", or "deprecated".
    std::string status;
    /// Human-readable description.
    std::optional<std::string> description;
    /// Input and output modalities.
    std::optional<Modalities> modalities;
    /// Request parameters the model honours.
    std::optional<std::vector<std::string>> supported_parameters;
    /// Training-data cutoff, as a date string.
    std::optional<std::string> knowledge_cutoff;
    /// Release date, as a date string.
    std::optional<std::string> released_at;
    /// Link to the model's documentation.
    std::optional<std::string> documentation_url;
    /// Alternative identifiers that resolve to this model.
    std::optional<std::vector<std::string>> aliases;
};

void to_json(json& j, const Model& m);
void from_json(const json& j, Model& m);

/// The response of @ref ModelsResource::list.
struct ModelList {
    /// Object type; always "list".
    std::string object;
    /// The flat list of models.
    std::vector<ModelSummary> data;
};

void to_json(json& j, const ModelList& l);
void from_json(const json& j, ModelList& l);

} // namespace skailar

#endif // SKAILAR_MODELS_HPP
