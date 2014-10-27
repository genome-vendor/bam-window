#include "TableBuilder.hpp"

#include "ColumnAssigner.hpp"
#include "RowAssigner.hpp"

#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <sstream>

namespace {
    struct MockEntry {
        uint32_t first_pos;
        uint32_t last_pos;
        uint32_t length;
        std::string read_group;
    };


    std::string name(MockEntry const&) {
        return "";
    }

    uint32_t first_pos(MockEntry const& e) {
        return e.first_pos;
    }

    uint32_t last_pos(MockEntry const& e) {
        return e.last_pos;
    }

    uint32_t length(MockEntry const& e) {
        return e.length;
    }

    char const* read_group(MockEntry const& e) {
        return e.read_group.c_str();
    }

    struct RowCollector {
        struct Row {
            std::string seq_name;
            uint32_t pos;
            std::vector<uint32_t> counts;

            uint32_t operator[](uint32_t idx) const {
                return counts[idx];
            }
        };

        void operator()(char const* seq, uint32_t p) {
            Row row;
            row.seq_name = seq;
            row.pos = p;
            rows.push_back(row);
        }

        void operator()(char const* seq, uint32_t p, std::vector<uint32_t> c) {
            Row row;
            row.seq_name = seq;
            row.pos = p;
            row.counts = std::move(c);
            rows.push_back(row);
        }

        std::vector<Row> rows;
    };
}

class TestTableBuilder : public ::testing::Test {
public:
    void SetUp() {
        uint32_t seq_len{62};
        uint32_t win_size{5};

        RgToLibMap rg2lib;
        rg2lib["rg1"] = "lib1";
        rg2lib["rg2"] = "lib1";
        rg2lib["rg3"] = "lib2";

        std::vector<uint32_t> read_lens{36, 150};

        row_assigner.reset(new RowAssigner(seq_len, win_size));
        col_assigner.reset(new PerLibAndLengthColumnAssigner(rg2lib, read_lens));
    }

    std::unique_ptr<RowAssigner> row_assigner;
    std::unique_ptr<ColumnAssignerBase> col_assigner;
};

TEST_F(TestTableBuilder, build) {
    RowCollector res;
    auto tb = make_table_builder("chr1", *row_assigner, *col_assigner, res);

    std::vector<MockEntry> entries{
          MockEntry{0, 4, 36, "rg1"}
        , MockEntry{2, 4, 36, "rg1"}
        , MockEntry{2, 14, 150, "rg2"}
        , MockEntry{30, 50, 36, "rg3"}
        };

    // Doesn't work on a compiler I have to support :(
    //std::for_each(entries.begin(), entries.end(), std::ref(tb));
    for (auto i = entries.begin(); i != entries.end(); ++i)
        tb(*i);

    tb.flush();

    auto const& rows = res.rows;
    ASSERT_EQ(13u, rows.size());

    // Make sure the positions go from 1->51 in steps of 5
    std::size_t idx{0};
    for (uint32_t pos = 1; pos <= 61; pos += 5, ++idx) {
        EXPECT_EQ(rows[idx].pos, pos);
        EXPECT_EQ("chr1", rows[idx].seq_name);
    }

    // First row, interval [1, 5]
    EXPECT_EQ(2u, rows[0][0]); // 2 36 bp reads for lib1 ([0, 4], [2, 4] rg1)
    EXPECT_EQ(1u, rows[0][1]); // 1 150 bp reads for lib1 ([2, 14] rg2)
    EXPECT_EQ(0u, rows[0][2]); // nothing for lib2 in [1, 5]
    EXPECT_EQ(0u, rows[0][3]);

    // [6-10]
    EXPECT_EQ(0u, rows[1][0]);
    EXPECT_EQ(1u, rows[1][1]); // ([2, 14] rg2)
    EXPECT_EQ(0u, rows[1][2]);
    EXPECT_EQ(0u, rows[1][3]);

    // [11-15]
    EXPECT_EQ(0u, rows[2][0]);
    EXPECT_EQ(1u, rows[2][1]); // ([2, 14] rg2)
    EXPECT_EQ(0u, rows[2][2]);
    EXPECT_EQ(0u, rows[2][3]);

    EXPECT_TRUE(rows[3].counts.empty()); // [16, 20] has no reads
    EXPECT_TRUE(rows[4].counts.empty()); // [21, 25]
    EXPECT_TRUE(rows[5].counts.empty()); // [26, 30]

    // The last 5 rows have the read from [30-50] in lib2, 36bp (col #3)
    for (uint32_t i = 6; i <= 9; ++i) {
        EXPECT_EQ(0u, rows[i][0]);
        EXPECT_EQ(0u, rows[i][1]);
        EXPECT_EQ(1u, rows[i][2]);
        EXPECT_EQ(0u, rows[i][3]);
    }

    EXPECT_TRUE(rows[10].counts.empty()); // [55, 60]
    EXPECT_TRUE(rows[11].counts.empty()); // [55, 60]
    EXPECT_TRUE(rows[12].counts.empty()); // [60, 65]
}
