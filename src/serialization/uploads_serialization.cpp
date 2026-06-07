#include <skailar/uploads.hpp>

#include "json_helpers.hpp"

#include <nlohmann/json.hpp>

namespace skailar {

const char* to_string(ImageContentType type) noexcept {
    switch (type) {
    case ImageContentType::Png:
        return "image/png";
    case ImageContentType::Jpeg:
        return "image/jpeg";
    case ImageContentType::Gif:
        return "image/gif";
    case ImageContentType::Webp:
        return "image/webp";
    }
    return "image/png";
}

const char* to_string(FileContentType type) noexcept {
    switch (type) {
    case FileContentType::Pdf:
        return "application/pdf";
    case FileContentType::Text:
        return "text/plain";
    }
    return "application/pdf";
}

void to_json(json& j, const UploadRequest& r) {
    j = json {{"base64", r.base64}, {"content_type", r.content_type}};
}

void from_json(const json& j, UploadRequest& r) {
    r.base64 = detail::get_string(j, "base64");
    r.content_type = detail::get_string(j, "content_type");
}

void to_json(json& j, const UploadResponse& r) {
    j = json {{"url", r.url}, {"content_type", r.content_type}};
}

void from_json(const json& j, UploadResponse& r) {
    r.url = detail::get_string(j, "url");
    r.content_type = detail::get_string(j, "content_type");
}

} // namespace skailar
