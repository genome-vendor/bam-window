#include "BamEntry.hpp"

char const* read_group(BamEntry const& e) {
    uint8_t const* rg = bam_aux_get(e, "RG");
    if (!rg)
        return 0;
    return reinterpret_cast<char const*>(rg + 1);
}

char const* name(BamEntry const& e) {
    return bam1_qname(e);
}


