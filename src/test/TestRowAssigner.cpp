#include "MockEntry.hpp"
#include "RowAssigner.hpp"

#include <gtest/gtest.h>

TEST(TestRowAssigner, num_wins) {
    RowAssigner ra(10, 1);
    EXPECT_EQ(10u, ra.num_wins);

    ra = RowAssigner(10, 3);
    EXPECT_EQ(4u, ra.num_wins);

    ra = RowAssigner(11, 2);
    EXPECT_EQ(6u, ra.num_wins);
}

TEST(TestRowAssigner, assign_start_and_stop_pos) {
    uint32_t seq_len = 20;
    uint32_t win_size = 9;

    RowAssigner ra(seq_len, win_size);
    ra.set_start_only(false);

    uint32_t fst = 1234;
    uint32_t lst = 5678;

    // spanning [0, 9) = 9 bases keeps us in the first window
    std::tie(fst, lst) = ra.row_range(MockEntry{0, 9});
    EXPECT_EQ(0u, fst);
    EXPECT_EQ(0u, lst);

    // if we go from [0, 10), we step into the second
    std::tie(fst, lst) = ra.row_range(MockEntry{0, 10});
    EXPECT_EQ(0u, fst);
    EXPECT_EQ(1u, lst);

    // let's try a few more
    std::tie(fst, lst) = ra.row_range(MockEntry{8, 9});
    EXPECT_EQ(0u, fst);
    EXPECT_EQ(0u, lst);

    std::tie(fst, lst) = ra.row_range(MockEntry{8, 10});
    EXPECT_EQ(0u, fst);
    EXPECT_EQ(1u, lst);

    std::tie(fst, lst) = ra.row_range(MockEntry{9, 10});
    EXPECT_EQ(1u, fst);
    EXPECT_EQ(1u, lst);

    std::tie(fst, lst) = ra.row_range(MockEntry{17, 19});
    EXPECT_EQ(1u, fst);
    EXPECT_EQ(2u, lst);

    // And now a special case, when we have [x, x) which
    // samtools sometimes reports
    std::tie(fst, lst) = ra.row_range(MockEntry{0, 0});
    EXPECT_EQ(0u, fst);
    EXPECT_EQ(0u, lst);

    std::tie(fst, lst) = ra.row_range(MockEntry{9, 9});
    EXPECT_EQ(1u, fst);
    EXPECT_EQ(1u, lst);

    EXPECT_EQ(0u, ra.start_pos_for_row(0));
    EXPECT_EQ(9u, ra.start_pos_for_row(1));
    EXPECT_EQ(18u, ra.start_pos_for_row(2));
}

TEST(TestRowAssigner, assign_start_pos_only) {
    uint32_t seq_len = 20;
    uint32_t win_size = 9;

    RowAssigner ra(seq_len, win_size);
    ra.set_start_only(true);

    uint32_t fst = 1234;
    uint32_t lst = 5678;

    // spanning [0, 8] = 9 bases keeps us in the first window
    std::tie(fst, lst) = ra.row_range(MockEntry{0, 8});
    EXPECT_EQ(0u, fst);
    EXPECT_EQ(0u, lst);

    // if we go from [0, 9], we step into the second
    std::tie(fst, lst) = ra.row_range(MockEntry{0, 9});
    EXPECT_EQ(0u, fst);
    EXPECT_EQ(0u, lst);

    // let's try a few more
    std::tie(fst, lst) = ra.row_range(MockEntry{8, 8});
    EXPECT_EQ(0u, fst);
    EXPECT_EQ(0u, lst);

    std::tie(fst, lst) = ra.row_range(MockEntry{8, 9});
    EXPECT_EQ(0u, fst);
    EXPECT_EQ(0u, lst);

    std::tie(fst, lst) = ra.row_range(MockEntry{9, 9});
    EXPECT_EQ(1u, fst);
    EXPECT_EQ(1u, lst);

    std::tie(fst, lst) = ra.row_range(MockEntry{17, 18});
    EXPECT_EQ(1u, fst);
    EXPECT_EQ(1u, lst);

    EXPECT_EQ(0u, ra.start_pos_for_row(0));
    EXPECT_EQ(9u, ra.start_pos_for_row(1));
    EXPECT_EQ(18u, ra.start_pos_for_row(2));
}
