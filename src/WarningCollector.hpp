#pragma once

#include "BamHeader.hpp"

#include <cstdint>
#include <iosfwd>
#include <string>
#include <unordered_map>

struct Options;

class WarningCollector {
public:
    WarningCollector(Options const& opts, RgToLibMap const& rg2lib);

    void warn_invalid_col(char const* rg, uint32_t len);
    void print(std::ostream& os);

private:
    Options const& opts_;
    RgToLibMap const& rg2lib_;

    std::size_t missing_rgs_;
    std::unordered_map<uint32_t, std::size_t> skipped_lens_;
    std::unordered_map<std::string, std::size_t> skipped_libs_;
    std::unordered_map<
          std::string
        , std::unordered_map<uint32_t, std::size_t>
        > lib_skipped_lengths_;
};
