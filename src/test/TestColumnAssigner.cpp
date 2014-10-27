#include "ColumnAssigner.hpp"

#include <gtest/gtest.h>

#include <sstream>

class TestColumnAssigner : public ::testing::Test {
public:
    void SetUp() {
        rg2lib["rg1"] = "lib1";
        rg2lib["rg2"] = "lib1";
        rg2lib["rg3"] = "lib2";
        rg2lib["rg4"] = "lib3";
    }

    RgToLibMap rg2lib;
};


TEST_F(TestColumnAssigner, init) {
    SingleColumnAssigner ca;
    EXPECT_EQ(1u, ca.num_columns());

    EXPECT_EQ(0, ca.assign_column("unknown_rg", 12));
    EXPECT_EQ(0, ca.assign_column("rg1", 10));
    EXPECT_EQ(0, ca.assign_column("rg2", 99));
    EXPECT_EQ(0, ca.assign_column("rg3", 18));
    EXPECT_EQ(0, ca.assign_column("rg4", 40));

    std::stringstream ss;
    ca.print_header(ss);
    EXPECT_EQ("Chr\tStart\tCounts\n", ss.str());
}


TEST_F(TestColumnAssigner, init_by_len) {
    std::vector<uint32_t> read_lens{20, 10};
    PerLengthColumnAssigner ca(read_lens);
    EXPECT_EQ(2u, ca.num_columns());

    for (uint32_t i = 0; i < 100; ++i) {
        if (i != 10 && i != 20) {
            EXPECT_EQ(-1, ca.assign_column("rg1", i));
            EXPECT_EQ(-1, ca.assign_column("rg2", i));
            EXPECT_EQ(-1, ca.assign_column("rg3", i));
            EXPECT_EQ(-1, ca.assign_column("rg4", i));
        }
    }

    EXPECT_EQ(0, ca.assign_column("rg1", 10));
    EXPECT_EQ(0, ca.assign_column("rg2", 10));
    EXPECT_EQ(0, ca.assign_column("rg3", 10));
    EXPECT_EQ(0, ca.assign_column("rg4", 10));

    EXPECT_EQ(1, ca.assign_column("rg1", 20));
    EXPECT_EQ(1, ca.assign_column("rg2", 20));
    EXPECT_EQ(1, ca.assign_column("rg3", 20));
    EXPECT_EQ(1, ca.assign_column("rg4", 20));

    std::stringstream ss;
    ca.print_header(ss);
    EXPECT_EQ("Chr\tStart\t10\t20\n", ss.str());
}


TEST_F(TestColumnAssigner, init_by_lib) {
    PerLibColumnAssigner ca(rg2lib);

    // 3 libs, no read lengths, should be 3 columns
    EXPECT_EQ(3u, ca.num_columns());
    std::vector<std::string> expected_cols{"lib1", "lib2", "lib3"};
    EXPECT_EQ(expected_cols, ca.column_names);

    std::stringstream ss;
    ca.print_header(ss);
    EXPECT_EQ("Chr\tStart\tlib1\tlib2\tlib3\n", ss.str());
}


TEST_F(TestColumnAssigner, assign_by_lib) {
    PerLibColumnAssigner ca(rg2lib);

    EXPECT_EQ(-1, ca.assign_column("unknown_rg", 100));

    // Read length shouldn't make a difference in this case
    for (uint32_t i = 0u; i < 100u; i += 10) {
        ASSERT_EQ(0, ca.assign_column("rg1", i));
        ASSERT_EQ(0, ca.assign_column("rg2", i));
        ASSERT_EQ(1, ca.assign_column("rg3", i));
        ASSERT_EQ(2, ca.assign_column("rg4", i));
    }
}


TEST_F(TestColumnAssigner, init_by_lib_and_len) {
    std::vector<uint32_t> read_lens{100, 36, 75};

    PerLibAndLengthColumnAssigner ca(rg2lib, read_lens);

    // 3 libs, 3 read lengths, should be 9 columns...
    EXPECT_EQ(9u, ca.num_columns());

    // with the following names in this exact order
    std::vector<std::string> expected_cols{
          "lib1.36", "lib1.75", "lib1.100"
        , "lib2.36", "lib2.75", "lib2.100"
        , "lib3.36", "lib3.75", "lib3.100"
        };

    EXPECT_EQ(expected_cols, ca.column_names);

    std::stringstream ss;
    ca.print_header(ss);
    EXPECT_EQ("Chr\tStart\t"
        "lib1.36\tlib1.75\tlib1.100\t"
        "lib2.36\tlib2.75\tlib2.100\t"
        "lib3.36\tlib3.75\tlib3.100\n"
        , ss.str());
}


TEST_F(TestColumnAssigner, assign_by_lib_and_len) {
    std::vector<uint32_t> read_lens{75, 36, 100};
    PerLibAndLengthColumnAssigner ca(rg2lib, read_lens);

    EXPECT_EQ(-1, ca.assign_column("unknown_rg", 100));

    std::set<uint32_t> read_set(read_lens.begin(), read_lens.end());

    // make sure we always get -1 for invalid read lengths
    for (uint32_t i = 0u; i < 150; ++i) {
        // skip read lengths that actually exist
        if (read_set.count(i))
            continue;

        EXPECT_EQ(-1, ca.assign_column("rg1", i));
        EXPECT_EQ(-1, ca.assign_column("rg2", i));
        EXPECT_EQ(-1, ca.assign_column("rg3", i));
        EXPECT_EQ(-1, ca.assign_column("rg4", i));
    }

    EXPECT_EQ(0, ca.assign_column("rg1", 36));
    EXPECT_EQ(1, ca.assign_column("rg1", 75));
    EXPECT_EQ(2, ca.assign_column("rg1", 100));

    // rg2 also goes to lib1; same cols as rg1
    EXPECT_EQ(0, ca.assign_column("rg2", 36));
    EXPECT_EQ(1, ca.assign_column("rg2", 75));
    EXPECT_EQ(2, ca.assign_column("rg2", 100));

    EXPECT_EQ(3, ca.assign_column("rg3", 36));
    EXPECT_EQ(4, ca.assign_column("rg3", 75));
    EXPECT_EQ(5, ca.assign_column("rg3", 100));

    EXPECT_EQ(6, ca.assign_column("rg4", 36));
    EXPECT_EQ(7, ca.assign_column("rg4", 75));
    EXPECT_EQ(8, ca.assign_column("rg4", 100));
}
