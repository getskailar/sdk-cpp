#ifndef SKAILAR_INTERNAL_STRING_UTILS_HPP
#define SKAILAR_INTERNAL_STRING_UTILS_HPP

#include <cctype>
#include <cstddef>
#include <string_view>

namespace skailar::detail {

inline char ascii_lower(char c) noexcept {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c - 'A' + 'a');
    }
    return c;
}

/// Case-insensitive ASCII string equality, used for header-name comparison.
inline bool iequals(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (ascii_lower(a[i]) != ascii_lower(b[i])) {
            return false;
        }
    }
    return true;
}

/// Trims leading and trailing ASCII whitespace.
inline std::string_view trim(std::string_view s) noexcept {
    std::size_t begin = 0;
    std::size_t end = s.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(s[begin])) != 0) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1])) != 0) {
        --end;
    }
    return s.substr(begin, end - begin);
}

} // namespace skailar::detail

#endif // SKAILAR_INTERNAL_STRING_UTILS_HPP
