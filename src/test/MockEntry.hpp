#pragma once

#include <cstdint>
#include <string>

struct MockEntry {
    uint32_t first_pos;
    uint32_t last_pos;
    uint32_t length;
    std::string read_group;
};

inline
std::string name(MockEntry const&) {
    return "";
}

inline
uint32_t first_pos(MockEntry const& e) {
    return e.first_pos;
}

inline
uint32_t last_pos(MockEntry const& e) {
    return e.last_pos;
}

inline
uint32_t length(MockEntry const& e) {
    return e.length;
}

inline
char const* read_group(MockEntry const& e) {
    return e.read_group.c_str();
}
