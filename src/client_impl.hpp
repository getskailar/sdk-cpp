#ifndef SKAILAR_CLIENT_IMPL_HPP
#define SKAILAR_CLIENT_IMPL_HPP

#include "http_client.hpp"

#include <skailar/client.hpp>
#include <skailar/config.hpp>

#include <string>
#include <utility>

#include <nlohmann/json.hpp>

namespace skailar {

/// The private implementation of @ref Client. Owns the shared HTTP context and
/// provides the typed request helpers the resource classes call.
class Client::Impl {
public:
    explicit Impl(ClientConfig config) : ctx_(std::move(config)) { }

    const detail::HttpContext& ctx() const noexcept { return ctx_; }

    /// Performs a GET and decodes the JSON response into T.
    template <typename T> T get_json(const std::string& path) const {
        detail::Response res
            = detail::do_request(ctx_, "GET", path, {}, detail::Idempotency::Idempotent);
        return decode<T>(res.body);
    }

    /// Performs a side-effecting POST with a JSON body and decodes the response.
    template <typename Req, typename Res>
    Res post_json(const std::string& path, const Req& request) const {
        const std::string body = nlohmann::json(request).dump();
        detail::Response res
            = detail::do_request(ctx_, "POST", path, body, detail::Idempotency::SideEffect);
        return decode<Res>(res.body);
    }

    /// Performs a side-effecting POST and returns the raw response body (for a
    /// non-JSON content type such as audio/mpeg).
    template <typename Req>
    std::string post_binary(const std::string& path, const Req& request) const {
        const std::string body = nlohmann::json(request).dump();
        detail::Response res
            = detail::do_request(ctx_, "POST", path, body, detail::Idempotency::SideEffect);
        return std::move(res.body);
    }

private:
    template <typename T> static T decode(const std::string& body) {
        nlohmann::json parsed = nlohmann::json::parse(body, nullptr, false);
        if (parsed.is_discarded()) {
            throw Error(ErrorKind::Decode, 0, "malformed response body", {}, {}, body);
        }
        try {
            return parsed.get<T>();
        } catch (const nlohmann::json::exception&) {
            throw Error(ErrorKind::Decode, 0, "malformed response body", {}, {}, body);
        }
    }

    detail::HttpContext ctx_;
};

} // namespace skailar

#endif // SKAILAR_CLIENT_IMPL_HPP
