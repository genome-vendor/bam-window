#pragma once

#include "BamEntry.hpp"
#include "BamHeader.hpp"

#include <sam.h>

#include <cstdint>
#include <memory>
#include <string>

struct BamFilter;

class BamReader {
public:
    explicit BamReader(std::string path);
    ~BamReader();

    void set_filter(BamFilter* filter);
    void set_sequence_idx(int32_t tid);

    bool next(BamEntry& entry);

    BamHeader const& header() const;

private:
    bool raw_next(BamEntry& entry);

private:
    std::string path_;
    samfile_t* in_;
    bam_index_t* index_;
    int32_t tid_;
    std::unique_ptr<BamHeader> header_;
    bam_iter_t iter_;

    BamFilter* filter_;
};
