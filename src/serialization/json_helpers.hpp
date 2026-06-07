#ifndef SKAILAR_SERIALIZATION_JSON_HELPERS_HPP
#define SKAILAR_SERIALIZATION_JSON_HELPERS_HPP

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace skailar::detail {

/// Writes @p value under @p key only when the optional holds a value, so unset
/// fields are omitted from the wire body entirely.
template <typename T>
void assign_optional(nlohmann::json& j, const char* key, const std::optional<T>& value) {
    if (value.has_value()) {
        j[key] = *value;
    }
}

/// Reads @p key from @p j into @p out when present and non-null, otherwise leaves
/// @p out empty.
template <typename T>
void read_optional(const nlohmann::json& j, const char* key, std::optional<T>& out) {
    auto it = j.find(key);
    if (it != j.end() && !it->is_null()) {
        out = it->get<T>();
    } else {
        out.reset();
    }
}

/// Reads a required string field, returning an empty string when absent.
inline std::string get_string(const nlohmann::json& j, const char* key) {
    auto it = j.find(key);
    if (it != j.end() && it->is_string()) {
        return it->get<std::string>();
    }
    return {};
}

} // namespace skailar::detail

#endif // SKAILAR_SERIALIZATION_JSON_HELPERS_HPP
