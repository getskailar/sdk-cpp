#ifndef SKAILAR_IMAGES_HPP
#define SKAILAR_IMAGES_HPP

#include <skailar/json_fwd.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace skailar {

/// A request to generate images from a prompt.
struct ImageGenerationRequest {
    /// Image model identifier, for example "gpt-image-1".
    std::string model;
    /// Text description of the desired image.
    std::string prompt;
    /// Number of images to generate (1-10).
    std::optional<int> n;
    /// Image size, for example "1024x1024", "1024x1792", "1792x1024".
    std::optional<std::string> size;
    /// Provider-specific quality hint, for example "standard", "hd".
    std::optional<std::string> quality;
    /// Provider-specific background hint, for example "transparent".
    std::optional<std::string> background;
};

void to_json(json& j, const ImageGenerationRequest& r);
void from_json(const json& j, ImageGenerationRequest& r);

/// One image within an @ref ImageGenerationResponse.
struct GeneratedImage {
    /// Hosted image URL, when the provider returns a URL.
    std::optional<std::string> url;
    /// Base64-encoded image bytes, when the provider returns inline data.
    std::optional<std::string> b64_json;
    /// The prompt as revised by the provider, when present.
    std::optional<std::string> revised_prompt;
};

void to_json(json& j, const GeneratedImage& g);
void from_json(const json& j, GeneratedImage& g);

/// The response of @ref ImagesResource::generate.
struct ImageGenerationResponse {
    /// Unix epoch seconds at generation.
    std::int64_t created = 0;
    /// The generated images.
    std::vector<GeneratedImage> data;
};

void to_json(json& j, const ImageGenerationResponse& r);
void from_json(const json& j, ImageGenerationResponse& r);

} // namespace skailar

#endif // SKAILAR_IMAGES_HPP
