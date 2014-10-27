#include "BamReader.hpp"
#include "BamFilter.hpp"

#include <boost/format.hpp>

#include <cassert>
#include <stdexcept>

using boost::format;

BamReader::BamReader(std::string path)
    : path_(std::move(path))
    , in_(samopen(path_.c_str(), "rb", 0))
    , index_(bam_index_load(path_.c_str()))
    , iter_(0)
{
    if (!in_ || !in_->header)
        throw std::runtime_error(str(format("Failed to open samfile %1%") % path_));

    if (!in_->x.bam)
        throw std::runtime_error(str(format("%1% is not a valid bam file") % path_));

    if (!index_)
        throw std::runtime_error(str(format("Failed to load bam index for %1%") % path_));

    header_.reset(new BamHeader(in_->header));

    set_sequence_idx(0);
}

BamReader::~BamReader() {
    if (in_)
        samclose(in_);

    if (index_)
        bam_index_destroy(index_);

    if (iter_)
        bam_iter_destroy(iter_);
}

void BamReader::set_filter(BamFilter* filter) {
    filter_ = filter;
}

void BamReader::set_sequence_idx(int32_t tid) {
    tid_ = tid;
    if (iter_)
        bam_iter_destroy(iter_);

    iter_ = bam_iter_query(index_, tid_, 0, header().seq_length(tid));
}

bool BamReader::raw_next(BamEntry& entry) {
    return bam_iter_read(in_->x.bam, iter_, entry) > 0;
}

bool BamReader::next(BamEntry& entry) {
    bool rv = false;
    while ((rv = raw_next(entry))) {
        if (filter_ && !filter_->want_entry(entry))
            continue;
        return rv;
    }
    return rv;
}

BamHeader const& BamReader::header() const {
    assert(header_);
    return *header_;
}
