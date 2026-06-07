#ifndef SKAILAR_JSON_FWD_HPP
#define SKAILAR_JSON_FWD_HPP

// nlohmann::json appears as a value-typed field (for example response_format and
// tool parameter schemas), including inside std::optional. std::optional<T>
// requires a complete T, so the full nlohmann/json definition is needed here
// rather than only a forward declaration.
#include <nlohmann/json.hpp>

namespace skailar {

/// Alias for nlohmann's JSON value, used for pass-through fields such as
/// `response_format`, `logit_bias`, and tool parameter schemas.
using json = nlohmann::json;

} // namespace skailar

#endif // SKAILAR_JSON_FWD_HPP
