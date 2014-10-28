#include "BamHeader.hpp"

#include <boost/format.hpp>

#include <sam_header.h>

#include <cassert>
#include <cstdlib>
#include <stdexcept>

using boost::format;

namespace {
    RgToLibMap rg2lib_map(bam_header_t* hdr) {
        assert(hdr != 0);

        RgToLibMap rv;
        static char rg_tag[] = "RG";
        static char id_tag[] = "ID";
        static char lb_tag[] = "LB";


        if (!hdr->dict)
            hdr->dict = sam_header_parse2(hdr->text);

        if (!hdr->rg2lib)
            hdr->rg2lib = sam_header2tbl(hdr->dict, rg_tag, id_tag, lb_tag);

        int n_rgs = 0;
        char** rgs = sam_header2list(hdr->dict, rg_tag, id_tag, &n_rgs);
        for (int i = 0; i < n_rgs; ++i) {
            char const* lib_name = sam_tbl_get(hdr->rg2lib, rgs[i]);
            if (!lib_name) {
                throw std::runtime_error(str(format(
                    "failed to get library name for read group %1%"
                    ) % rgs[i]));
            }

            rv[rgs[i]] = lib_name;
        }

        free(rgs);

        return rv;
    }
}

BamHeader::BamHeader(bam_header_t* header)
    : header_(header)
    , rg_to_lib_(rg2lib_map(header))
{
    for (int32_t i = 0; i < header_->n_targets; ++i) {
        seq_name_to_idx_[seq_name(i)] = i;
    }
}

int32_t BamHeader::num_seqs() const {
    return header_->n_targets;
}

char const* BamHeader::seq_name(int32_t seq_idx) const {
    assert(seq_idx >= 0 && seq_idx < num_seqs());
    return header_->target_name[seq_idx];
}

uint32_t BamHeader::seq_length(int32_t seq_idx) const {
    assert(seq_idx >= 0 && seq_idx < num_seqs());
    return header_->target_len[seq_idx];
}

auto BamHeader::rg_to_lib_map() const -> RgToLibMap const& {
    return rg_to_lib_;
}

int32_t BamHeader::seq_idx(std::string const& seq_name) const {
    auto found = seq_name_to_idx_.find(seq_name);
    if (found == seq_name_to_idx_.end())
        return -1;
    return found->second;
}
