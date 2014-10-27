#include "ColumnAssigner.hpp"
#include "BamEntry.hpp"
#include "BamHeader.hpp"
#include "BamReader.hpp"
#include "Options.hpp"

#include <boost/lexical_cast.hpp>

#include <algorithm>
#include <cassert>
#include <unordered_set>

namespace {
    std::vector<uint32_t> get_read_lengths(
              BamReader& reader
            , std::size_t max_entries
            )
    {
        BamEntry e;
        std::unordered_set<uint32_t> lens;

        for (std::size_t i = 0; i < max_entries && reader.next(e); ++i) {
            lens.insert(length(e));
        }

        return std::vector<uint32_t>(lens.begin(), lens.end());
    }
}

// All of the switching based on command line flags (report by lib, len) is
// now collected here in this function.
std::unique_ptr<ColumnAssignerBase> make_column_assigner(
          Options const& opts
        , BamReader& reader
        )
{
    typedef std::unique_ptr<ColumnAssignerBase> RV;

    auto const& header = reader.header();
    if (opts.per_read_len) {
        auto read_lens = get_read_lengths(reader, 1000000);

        if (opts.per_lib)
            return RV{new PerLibAndLengthColumnAssigner(header.rg_to_lib_map(), read_lens)};
        else
            return RV{new PerLengthColumnAssigner(read_lens)};
    }
    else {
        if (opts.per_lib)
            return RV{new PerLibColumnAssigner(header.rg_to_lib_map())};
        else
            return RV{new SingleColumnAssigner};
    }
}


//////////////////////////////////////////////////////////////////////
// Per Lib
PerLibColumnAssigner::PerLibColumnAssigner(RgToLibMap const& rg2lib) {
    boost::container::flat_set<std::string> lib_names;
    for (auto i = rg2lib.begin(); i != rg2lib.end(); ++i)
        lib_names.insert(i->second);

    for (auto i = rg2lib.begin(); i != rg2lib.end(); ++i) {
        auto where = lib_names.find(i->second);
        assert(where != lib_names.end());
        rg_to_col[i->first] = std::distance(lib_names.begin(), where);
    }
    column_names.assign(lib_names.begin(), lib_names.end());
}

std::size_t PerLibColumnAssigner::num_columns() const {
    return column_names.size();
}

int PerLibColumnAssigner::assign_column(char const* rg, uint32_t read_len) const {
    auto iter = rg_to_col.find(rg);
    if (iter == rg_to_col.end())
        return -1;
    return iter->second;
}


//////////////////////////////////////////////////////////////////////
// Per Length
PerLengthColumnAssigner::PerLengthColumnAssigner(std::vector<uint32_t> const& lens)
    : read_lens(lens.begin(), lens.end())
{
    for (auto i = read_lens.begin(); i != read_lens.end(); ++i) {
        column_names.push_back(boost::lexical_cast<std::string>(*i));
    }
}

int PerLengthColumnAssigner::assign_column(char const* rg, uint32_t read_len) const {
    auto found = read_lens.find(read_len);
    if (found == read_lens.end())
        return -1;
    return found - read_lens.begin();
}

//////////////////////////////////////////////////////////////////////
// Per Lib and Length
PerLibAndLengthColumnAssigner::PerLibAndLengthColumnAssigner(
          RgToLibMap const& rg2lib
        , std::vector<uint32_t> const& rl
        )
{
    // First, let's sort and deduplicate the lib names and read lengths
    boost::container::flat_set<uint32_t> read_lens(rl.begin(), rl.end());
    boost::container::flat_set<std::string> lib_names;
    for (auto i = rg2lib.begin(); i != rg2lib.end(); ++i)
        lib_names.insert(i->second);

    // Column names get set as <lib_name>.<read_length>
    // Read length varies the fastest, so we get things like:
    //  lib1.len1, lib1.len2, lib2.len1, lib2.len2, ...
    std::size_t n_cols{rl.size() * lib_names.size()};
    column_names.reserve(n_cols);
    for (auto i = lib_names.begin(); i != lib_names.end(); ++i) {
        auto const& lib = *i;
        for (auto j = read_lens.begin(); j != read_lens.end(); ++j) {
            using boost::lexical_cast;
            auto len = *j;
            column_names.push_back(lib + "." + lexical_cast<std::string>(len));
        }
    }

    // Now we can build our index mapping (rg, len) -> column_index
    std::size_t stride = read_lens.size(); // this many columns for each lib
    for (auto i = rg2lib.begin(); i != rg2lib.end(); ++i) {
        auto const& rg = i->first;
        auto const& lib = i->second;
        auto lib_iter = lib_names.find(lib);
        assert(lib_iter != lib_names.end());
        auto lib_idx = std::distance(lib_names.begin(), lib_iter);

        for (auto j = read_lens.begin(); j != read_lens.end(); ++j) {
            std::size_t len_idx = std::distance(read_lens.begin(), j);
            std::size_t idx = stride * lib_idx + len_idx;
            KeyType key{rg, *j};
            index[key] = idx;
        }
    }
}

int PerLibAndLengthColumnAssigner::assign_column(const char* rg, uint32_t read_len) const {
    KeyType key{rg, read_len};
    auto found = index.find(key);
    if (found == index.end())
        return -1;
    return found->second;
}
