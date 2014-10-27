#pragma once

#include "Options.hpp"

struct BamFilter {
    BamFilter(Options const& opts)
        : opts_(opts)
    {}

    template<typename T>
    bool want_entry(T const& e) const {
        int mapq = mapping_quality(e);
        int flag = sam_flag(e);
        return mapq >= opts_.min_mapq
            && (flag & opts_.required_flags) == opts_.required_flags
            && (flag & opts_.forbidden_flags) == 0;
    }

    Options const& opts_;
};

