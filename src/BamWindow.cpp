#include "BamWindow.hpp"

#include "BamEntry.hpp"
#include "BamFilter.hpp"
#include "BamHeader.hpp"
#include "BamReader.hpp"
#include "ColumnAssigner.hpp"
#include "RowAssigner.hpp"
#include "TableBuilder.hpp"

#include <boost/format.hpp>

#include <cassert>
#include <chrono>
#include <iostream>
#include <unordered_set>

using boost::format;

namespace {
    std::vector<int32_t> configure_sequences(
          std::vector<std::string> seq_names
        , BamHeader const& header
        )
    {
        std::vector<int32_t> rv;
        if (seq_names.empty()) {
            rv.reserve(header.num_seqs());
            for (int32_t i = 0; i < header.num_seqs(); ++i) {
                rv.push_back(i);
            }
        }
        else {
            rv.reserve(seq_names.size());
            for (auto i = seq_names.begin(); i != seq_names.end(); ++i) {
                int32_t idx = header.seq_idx(*i);
                if (idx < 0) {
                    throw std::runtime_error(str(format(
                        "Sequence %1% not found in bam file."
                        ) % *i));
                }
                rv.push_back(idx);
            }
        }
        return rv;
    }
}

BamWindow::BamWindow(Options const& opts)
    : opts_(opts)
{
    open_output_file();
}

void BamWindow::open_output_file() {
    if (!opts_.output_file.empty() && opts_.output_file != "-") {
        output_file_ptr_.reset(new std::ofstream(opts_.output_file));
        if (!output_file_ptr_->is_open()) {
            throw std::runtime_error(str(format(
                "Failed to open output file %1%"
                ) % opts_.output_file));
        }
        out_ptr_ = output_file_ptr_.get();
    }
    else {
        out_ptr_ = &std::cout;
    }
}

bool BamWindow::configure_downsampling() const {
    bool downsampling = opts_.downsample < 1.0f;
    if (downsampling) {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(now);
        long seed = msec.count();

        if (!opts_.seed_string.empty())
            seed = opts_.seed;

        srand48(seed);
        std::clog << "RNG seed: " << seed << "\n";
    }
    return downsampling;
}

void BamWindow::exec() {
    BamFilter filter(opts_);
    BamReader reader(opts_.input_file);
    reader.set_filter(&filter);

    auto const& header = reader.header();

    std::unique_ptr<ColumnAssignerBase> col_assigner = make_column_assigner(opts_, reader);
    col_assigner->print_header(*out_ptr_);

    DefaultRowPrinter printer(*out_ptr_, *col_assigner);
    bool downsample = configure_downsampling();

    BamEntry e;
    auto seqs = configure_sequences(opts_.sequence_names, header);
    for (auto iter = seqs.begin(); iter != seqs.end(); ++iter) {
        auto i = *iter;
        reader.set_sequence_idx(i);
        char const* seq_name = header.seq_name(i);
        assert(seq_name != 0);
        uint32_t seq_len = header.seq_length(i);
        RowAssigner row_assigner(seq_len, opts_.window_size);
        row_assigner.set_start_only(opts_.leftmost);
        TableBuilder<> builder(seq_name, row_assigner, *col_assigner, printer);

        while (reader.next(e)) {
            if (!downsample || (drand48() < opts_.downsample))
                builder(e);
        }
    }
}
