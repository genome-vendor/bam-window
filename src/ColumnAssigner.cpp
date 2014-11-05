#include "ColumnAssigner.hpp"
#include "BamEntry.hpp"
#include "BamHeader.hpp"
#include "BamReader.hpp"
#include "Options.hpp"

#include <boost/lexical_cast.hpp>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace {
    PerLibReadLengths get_per_lib_read_lengths(
          BamReader& reader
        , std::size_t max_entries
        )
    {
        auto const& rg2lib = reader.header().rg_to_lib_map();
        BamEntry e;
        PerLibReadLengths lens;

        reader.clear_region();
        for (std::size_t i = 0; i < max_entries && reader.next(e); ++i) {
            char const* rg = read_group(e);
            if (!rg)
                continue;

            auto iter = rg2lib.find(rg);
            if (iter == rg2lib.end())
                continue;

            auto const& lib = iter->second;
            lens[lib].insert(length(e));
        }

        if (lens.empty()) {
            throw std::runtime_error(
                "Unable to determine read lengths by library. "
                "Are RG tags missing?");
        }

        return lens;
    }

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
        std::size_t first_n_reads = 1000000;

        if (opts.per_lib) {
            auto read_lens = get_per_lib_read_lengths(reader, first_n_reads);
            return RV{new PerLibAndLengthColumnAssigner(header.rg_to_lib_map(), read_lens)};
        }
        else {
            auto read_lens = get_read_lengths(reader, first_n_reads);
            return RV{new PerLengthColumnAssigner(read_lens)};
        }
    }
    else {
        if (opts.per_lib)
            return RV{new PerLibColumnAssigner(header.rg_to_lib_map())};
        else
            return RV{new SingleColumnAssigner};
    }
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
// Per Lib
PerLibColumnAssigner::PerLibColumnAssigner(RgToLibMap rg2lib)
    : rg2lib_(std::move(rg2lib))
{
    boost::container::flat_set<std::string> lib_names;
    for (auto i = rg2lib_.begin(); i != rg2lib_.end(); ++i)
        lib_names.insert(i->second);

    for (auto i = rg2lib_.begin(); i != rg2lib_.end(); ++i) {
        auto where = lib_names.find(i->second);
        assert(where != lib_names.end());
        auto offset = std::distance(lib_names.begin(), where);
        KeyType key{i->first.c_str()};
        index_[key] = offset;
    }
    column_names.assign(lib_names.begin(), lib_names.end());
}

std::size_t PerLibColumnAssigner::num_columns() const {
    return column_names.size();
}

int PerLibColumnAssigner::assign_column(char const* rg, uint32_t read_len) const {
    if (!rg)
        return -1;

    auto iter = index_.find(rg);
    if (iter == index_.end())
        return -1;
    return iter->second;
}


//////////////////////////////////////////////////////////////////////
// Per Lib and Length
PerLibAndLengthColumnAssigner::PerLibAndLengthColumnAssigner(
          RgToLibMap rg2lib
        , PerLibReadLengths const& read_lens
        )
    : rg2lib_(std::move(rg2lib))
{
    // lib_name -> first column index for that lib
    // (each lib can have separate columns for each of its read lengths)
    std::unordered_map<std::string, std::size_t> lib_starts;

    // populate lib_starts and set column names
    for (auto i = read_lens.begin(); i != read_lens.end(); ++i) {
        auto const& lib_name = i->first;
        auto const& lens = i->second;
        lib_starts[lib_name] = column_names.size();

        for (auto j = lens.begin(); j != lens.end(); ++j) {
            column_names.push_back(lib_name + "." + boost::lexical_cast<std::string>(*j));
        }
    }


    // Now we can build our index mapping (rg, len) -> column_index
    for (auto i = rg2lib_.begin(); i != rg2lib_.end(); ++i) {
        auto const& rg = i->first;
        auto const& lib = i->second;

        auto lib_iter = read_lens.find(lib);
        if (lib_iter == read_lens.end()) {
            std::cerr << "WARNING: no read lengths found for read group " << rg << "\n";
            continue;
        }

        // if it's in read_lens, it should be in lib_starts too
        assert(lib_starts.count(lib) != 0);
        std::size_t lib_idx = lib_starts[lib];

        auto const& lens = lib_iter->second;

        for (auto j = lens.begin(); j != lens.end(); ++j) {
            std::size_t len_idx = std::distance(lens.begin(), j);
            std::size_t idx = lib_idx + len_idx;
            KeyType key{rg.c_str(), *j};
            index_[key] = idx;
        }
    }
}

int PerLibAndLengthColumnAssigner::assign_column(const char* rg, uint32_t read_len) const {
    if (!rg)
        return -1;

    KeyType key{rg, read_len};
    auto found = index_.find(key);
    if (found == index_.end())
        return -1;
    return found->second;
}
