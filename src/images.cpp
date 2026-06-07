#include <skailar/client.hpp>

#include "client_impl.hpp"

#include <skailar/images.hpp>

namespace skailar {

ImageGenerationResponse ImagesResource::generate(const ImageGenerationRequest& request) const {
    return client_->impl()->post_json<ImageGenerationRequest, ImageGenerationResponse>(
        "v1/images/generations", request);
}

} // namespace skailar
