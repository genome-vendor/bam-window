#pragma once

#include "RowAssigner.hpp"
#include "ColumnAssigner.hpp"

#include <algorithm>
#include <cassert>
#include <deque>
#include <iostream>
#include <vector>
#include <tuple>

struct DefaultRowPrinter {
    DefaultRowPrinter(std::ostream& os, ColumnAssignerBase const& col_assigner)
        : os(os)
    {
        std::size_t n_cols = col_assigner.num_columns();
        empty_value_str.reserve(2 * n_cols);
        for (std::size_t i = 0; i < n_cols; ++i) {
            empty_value_str += "\t0";
        }
    }

    void operator()(
              char const* seq_name
            , uint32_t pos
            ) const
    {
        os << seq_name << "\t" << pos << empty_value_str << "\n";
    }

    void operator()(
              char const* seq_name
            , uint32_t pos
            , std::vector<uint32_t> const& counts
            ) const
    {
        os << seq_name << "\t" << pos;
        for (auto i = counts.begin(); i != counts.end(); ++i) {
            os << "\t" << *i;
        }
        os << "\n";
    }

    std::ostream& os;
    std::string empty_value_str;
};

template<typename PrinterType = DefaultRowPrinter>
class TableBuilder {
public:
    typedef std::vector<uint32_t> Counts;

    // seq_name is expected to outlive this object.
    // In practice it comes from the bam header, so this is not an issue.
    TableBuilder(
              char const* seq_name
            , RowAssigner const& row_assigner
            , ColumnAssignerBase const& col_assigner
            , PrinterType& printer
            )
        : current_row_(0)
        , seq_name_(seq_name)
        , row_assigner_(row_assigner)
        , col_assigner_(col_assigner)
        , printer_(printer)
        , needs_read_group_(col_assigner_.needs_read_group())
    {
    }

    ~TableBuilder() {
        flush();
    }

    template<typename T>
    void operator()(T const& value) {
        uint32_t fstpos = first_pos(value);
        uint32_t lstpos = last_pos(value);

        uint32_t fst_row, lst_row;
        std::tie(fst_row, lst_row) = row_assigner_.row_range(fstpos, lstpos);
        char const* rg{0};
        if (needs_read_group_)
            rg = read_group(value);

        uint32_t len = length(value);
        int col = col_assigner_.assign_column(rg, len);

        if (col < 0) {
            std::cerr << "Warning: read " << name(value)
                << " has invalid read group or length ("
                << rg << ", " << len << "), skipping\n";
            return;
        }

        set_current_row(fst_row);

        for (auto i = fst_row; i <= lst_row; ++i) {
            increment_cell(i, col);
        }
    }

    void increment_cell(uint32_t idx, uint32_t col) {
        assert(idx >= current_row_);
        uint32_t local_idx = idx - current_row_;
        while (local_idx >= rows_.size()) {
            rows_.push_back(new_row());
        }
        ++rows_[local_idx][col];
    }

    void set_current_row(uint32_t idx) {
        if (idx > current_row_) {
            advance_to(idx);
        }
    }

    void advance_to(uint32_t idx) {
        assert(idx >= current_row_);
        uint32_t diff = idx - current_row_;
        uint32_t n_with_data = std::min(diff, uint32_t(rows_.size()));
        for (uint32_t i = 0; i < n_with_data; ++i, ++current_row_) {
            print_row(rows_.front());
            rows_.pop_front();
        }

        for (uint32_t i = n_with_data; i < diff; ++i, ++current_row_) {
            print_empty_row();
        }
    }

    void print_empty_row() const {
        uint32_t pos = row_assigner_.win_size * current_row_ + 1;
        printer_(seq_name_, pos);
    }

    void print_row(Counts const& c) const {
        assert(c.size() == col_assigner_.num_columns());

        // convert to 1-based output coordinates
        uint32_t pos = row_assigner_.win_size * current_row_ + 1;
        printer_(seq_name_, pos, c);
    }

    Counts new_row() const {
        return Counts(col_assigner_.num_columns(), 0u);
    }

    void flush() {
        for (auto i = rows_.begin(); i != rows_.end(); ++i) {
            print_row(rows_.front());
            rows_.pop_front();
            ++current_row_;
        }

        for (; current_row_ < row_assigner_.num_wins; ++current_row_) {
            print_empty_row();
        }
    }

private:
    uint32_t current_row_;
    char const* seq_name_;
    RowAssigner const& row_assigner_;
    ColumnAssignerBase const& col_assigner_;
    PrinterType& printer_;
    bool needs_read_group_;
    std::deque<Counts> rows_;
};


template<typename PrinterType>
TableBuilder<PrinterType>
make_table_builder(
          char const* seq_name
        , RowAssigner const& row_assigner
        , ColumnAssignerBase const& col_assigner
        , PrinterType& printer
        )
{
    return TableBuilder<PrinterType>(
        seq_name, row_assigner, col_assigner, printer
        );
}
