#ifndef SKAILAR_PING_HPP
#define SKAILAR_PING_HPP

#include <skailar/json_fwd.hpp>

#include <string>

namespace skailar {

/// The response of @ref Client::ping.
struct PingKeyResponse {
    /// Always "ok" when the key is valid.
    std::string status;
    /// The Skailar user id that owns the key.
    std::string user_id;
};

void to_json(json& j, const PingKeyResponse& r);
void from_json(const json& j, PingKeyResponse& r);

} // namespace skailar

#endif // SKAILAR_PING_HPP
