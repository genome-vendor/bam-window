#include "BamEntry.hpp"

uint32_t first_pos(BamEntry const& e) {
    return e->core.pos;
}

uint32_t last_pos(BamEntry const& e) {
    return bam_calend(&e->core, bam1_cigar(e));
}

char const* read_group(BamEntry const& e) {
    uint8_t const* rg = bam_aux_get(e, "RG");
    if (!rg)
        return 0;
    return reinterpret_cast<char const*>(rg + 1);
}

uint32_t length(BamEntry const& e) {
    return e->core.l_qseq;
}

char const* name(BamEntry const& e) {
    return bam1_qname(e);
}

int mapping_quality(BamEntry const& e) {
    return e->core.qual;
}

int sam_flag(BamEntry const& e) {
    return e->core.flag;
}
