#include <skailar/ping.hpp>

#include "serialization/json_helpers.hpp"

#include <nlohmann/json.hpp>

namespace skailar {

void to_json(json& j, const PingKeyResponse& r) {
    j = json {{"status", r.status}, {"user_id", r.user_id}};
}

void from_json(const json& j, PingKeyResponse& r) {
    r.status = detail::get_string(j, "status");
    r.user_id = detail::get_string(j, "user_id");
}

} // namespace skailar
