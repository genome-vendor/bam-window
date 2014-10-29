#pragma once

#include <cassert>
#include <cstdint>
#include <tuple>

struct RowAssigner {
    RowAssigner(uint32_t seq_len, uint32_t win_size);

    void set_start_only(bool value) {
        start_only = value;
    }

    // Get the range of rows this observation applies to
    // first/last_pos are 0-based [first_pos, last_pos)
    std::tuple<uint32_t, uint32_t> row_range(
              uint32_t first_pos
            , uint32_t last_pos
            ) const;

    template<typename T>
    std::tuple<uint32_t, uint32_t> row_range(T const& value) const {
        uint32_t fst_pos = first_pos(value);
        uint32_t first_row = fst_pos / win_size;

        if (start_only)
            return std::make_tuple(first_row, first_row);

        uint32_t lst_pos = last_pos(value);
        assert(fst_pos <= lst_pos);

        // sometimes we get 0 length reads reported as [123, 123)
        // that doesn't really make sense, so we don't let the last
        // row go below the first here.
        if (lst_pos == fst_pos)
            ++lst_pos;
        uint32_t last_row = (lst_pos - 1) / win_size;
        assert(last_row >= first_row);
        return std::make_tuple(first_row, last_row);
    }

    uint32_t start_pos_for_win_index(uint32_t idx) const {
        return idx * win_size;
    }

    uint32_t seq_len;
    uint32_t win_size;
    uint32_t num_wins;
    bool start_only;
};
