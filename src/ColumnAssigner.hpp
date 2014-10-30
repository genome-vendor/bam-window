#pragma once

#include "BamHeader.hpp"
#include "MurmurHash2.hpp"

#include <boost/container/flat_set.hpp>
#include <boost/functional/hash.hpp>

#include <cstdint>
#include <cstring>
#include <memory>
#include <ostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

struct Options;
class BamReader;

// There are currently 4 modes for mapping (read_group, read_length) -> output
// column:
//      Use a single column.
//      Report by library.
//      Report by read length.
//      Report by library x read_length.
//
// ColumnAssignerBase is the base class for all of these methods.
// All child classes are expected to fill in the column_names vector OR
// override the num_columns / print_header virtual functions.
struct ColumnAssignerBase {
    std::vector<std::string> column_names;

    virtual ~ColumnAssignerBase() {}

    virtual int assign_column(char const* rg, uint32_t read_len) const = 0;
    // Finding the read group is a bit expensive; it is not ideal that we
    // always require it to be passed. For now, we expose 'needs_read_group'
    // to clients so they know whether or not it will be used (if not, they
    // are free to pass nullptr for rg.
    virtual bool needs_read_group() const = 0;

    virtual std::size_t num_columns() const { return column_names.size(); }
    virtual void print_header(std::ostream& os) const {
        os << "Chr\tStart";
        for (auto i = column_names.begin(); i != column_names.end(); ++i) {
            os << "\t" << *i;
        }
        os << "\n";
    }
};

// Factory function to construct the right column assigner given the command
// line options (see Options.hpp; the relevant flags are per_lib and
// per_read_len)
std::unique_ptr<ColumnAssignerBase> make_column_assigner(
          Options const& opts
        , BamReader& reader
        );

// The implementations:

struct SingleColumnAssigner : ColumnAssignerBase {
    SingleColumnAssigner() {
        column_names.push_back("Counts");
    }

    int assign_column(char const* rg, uint32_t read_len) const { return 0; }
    bool needs_read_group() const { return false; }
};

struct PerLengthColumnAssigner : ColumnAssignerBase {
    explicit PerLengthColumnAssigner(std::vector<uint32_t> const& lens);

    int assign_column(char const* rg, uint32_t read_len) const;
    bool needs_read_group() const { return false; }

    boost::container::flat_set<uint32_t> read_lens;
};

struct PerLibColumnAssigner : ColumnAssignerBase {
    explicit PerLibColumnAssigner(RgToLibMap rg2lib);

    std::size_t num_columns() const;
    int assign_column(char const* rg, uint32_t read_len) const;
    bool needs_read_group() const { return true; }

private:
    struct KeyType {
        KeyType(char const* x) : rg(x) {}
        char const* rg;
        bool operator==(KeyType const& rhs) const {
            return strcmp(rg, rhs.rg) == 0;
        }
    };

    struct KeyHasher {
        std::size_t operator()(KeyType const& x) const {
            assert(x.rg != 0);
            char const* p = x.rg;
            return murmurhash2(p, strlen(p), 40);
        }
    };


    RgToLibMap rg2lib_;
    std::unordered_map<KeyType, uint32_t, KeyHasher> index_;
};

struct PerLibAndLengthColumnAssigner : ColumnAssignerBase {
    PerLibAndLengthColumnAssigner(
              RgToLibMap rg2lib
            , std::vector<uint32_t> const& read_lens
            );

    int assign_column(char const* rg, uint32_t read_len) const;
    bool needs_read_group() const { return true; }

private:
    struct KeyType {
        char const* rg;
        uint32_t len;

        bool operator==(KeyType const& rhs) const {
            return strcmp(rg, rhs.rg) == 0 && len == rhs.len;
        }
    };

    struct KeyHasher {
        std::size_t operator()(KeyType const& x) const {
            assert(x.rg != 0);
            char const* p = x.rg;
            std::size_t seed = murmurhash2(p, strlen(p), x.len);
            return seed;
        }
    };


    RgToLibMap rg2lib_;
    std::unordered_map<KeyType, uint32_t, KeyHasher> index_;
};
