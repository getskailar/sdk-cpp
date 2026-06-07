#ifndef SKAILAR_UPLOADS_HPP
#define SKAILAR_UPLOADS_HPP

#include <skailar/json_fwd.hpp>

#include <cstdint>
#include <string>

namespace skailar {

/// The MIME type of an image uploaded via @ref ImageUploadsResource::create.
enum class ImageContentType : std::uint8_t {
    /// image/png.
    Png,
    /// image/jpeg.
    Jpeg,
    /// image/gif.
    Gif,
    /// image/webp.
    Webp,
};

/// Returns the wire string for @p type (for example "image/png").
const char* to_string(ImageContentType type) noexcept;

/// The MIME type of a document uploaded via @ref FileUploadsResource::create.
enum class FileContentType : std::uint8_t {
    /// application/pdf.
    Pdf,
    /// text/plain.
    Text,
};

/// Returns the wire string for @p type (for example "application/pdf").
const char* to_string(FileContentType type) noexcept;

/// The wire body for the upload endpoints.
struct UploadRequest {
    /// Base64-encoded payload, without a `data:` prefix.
    std::string base64;
    /// MIME type of the payload.
    std::string content_type;
};

void to_json(json& j, const UploadRequest& r);
void from_json(const json& j, UploadRequest& r);

/// The response of the upload endpoints.
struct UploadResponse {
    /// Skailar-relative URL of the stored asset, ready to embed in subsequent
    /// calls.
    std::string url;
    /// The stored MIME type.
    std::string content_type;
};

void to_json(json& j, const UploadResponse& r);
void from_json(const json& j, UploadResponse& r);

} // namespace skailar

#endif // SKAILAR_UPLOADS_HPP
