#include "RowAssigner.hpp"

#include <cassert>

RowAssigner::RowAssigner(uint32_t seq_len, uint32_t win_size)
    : seq_len(seq_len)
    , win_size(win_size)
    , num_wins(1 + (seq_len - 1) / win_size)
    , start_only(false)
{
}

std::tuple<uint32_t, uint32_t> RowAssigner::row_range(
          uint32_t first_pos
        , uint32_t last_pos
        ) const
{
    assert(first_pos <= last_pos);

    uint32_t first_row = first_pos / win_size;

    if (start_only)
        return std::make_tuple(first_row, first_row);

    // sometimes we get 0 length reads reported as [123, 123)
    // that doesn't really make sense, so we don't let the last
    // row go below the first here.
    if (last_pos == first_pos)
        ++last_pos;
    uint32_t last_row = (last_pos - 1) / win_size;
    assert(last_row >= first_row);
    return std::make_tuple(first_row, last_row);
}
