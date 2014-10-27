#pragma once

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

    uint32_t start_pos_for_win_index(uint32_t idx) const {
        return idx * win_size;
    }

    uint32_t seq_len;
    uint32_t win_size;
    uint32_t num_wins;
    bool start_only;
};
