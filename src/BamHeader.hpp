#pragma once

#include <boost/container/flat_map.hpp>

#include <sam.h>

#include <string>

typedef boost::container::flat_map<std::string, std::string> RgToLibMap;

class BamHeader {
public:

    BamHeader(bam_header_t* header);

    int32_t num_seqs() const;
    char const* seq_name(int32_t seq_idx) const;
    uint32_t seq_length(int32_t seq_idx) const;
    RgToLibMap const& rg_to_lib_map() const;

private:
    bam_header_t* header_;
    RgToLibMap rg_to_lib_;
};
