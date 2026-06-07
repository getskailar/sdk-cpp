#include <skailar/client.hpp>

#include "client_impl.hpp"

#include <skailar/models.hpp>

#include <string>

namespace skailar {

ModelList ModelsResource::list() const {
    return client_->impl()->get_json<ModelList>("v1/models");
}

Model ModelsResource::retrieve(const std::string& id) const {
    // The id may contain slashes (for example "google/gemini-2.5-pro"); they are
    // kept as path separators, matching the gateway routing.
    return client_->impl()->get_json<Model>("v1/models/" + id);
}

} // namespace skailar
