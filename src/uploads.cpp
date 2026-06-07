#include <skailar/client.hpp>

#include "client_impl.hpp"

#include <skailar/uploads.hpp>

#include <string>

namespace skailar {

UploadResponse ImageUploadsResource::create(const std::string& base64_data,
                                            ImageContentType content_type) const {
    UploadRequest request;
    request.base64 = base64_data;
    request.content_type = to_string(content_type);
    return client_->impl()->post_json<UploadRequest, UploadResponse>("v1/uploads/images", request);
}

UploadResponse FileUploadsResource::create(const std::string& base64_data,
                                           FileContentType content_type) const {
    UploadRequest request;
    request.base64 = base64_data;
    request.content_type = to_string(content_type);
    return client_->impl()->post_json<UploadRequest, UploadResponse>("v1/uploads/files", request);
}

} // namespace skailar
