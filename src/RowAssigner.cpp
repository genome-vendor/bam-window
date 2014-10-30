#include "RowAssigner.hpp"

#include <cassert>

RowAssigner::RowAssigner(uint32_t seq_len, uint32_t win_size)
    : win_size(win_size)
    , start_only(false)
    , seq_len(seq_len)
    , num_wins(1 + (seq_len - 1) / win_size)
{
}

uint32_t RowAssigner::start_pos_for_row(uint32_t idx) const {
    return idx * win_size;
}
