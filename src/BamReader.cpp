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
    , total_(0)
    , filtered_(0)
{
    if (!in_ || !in_->header)
        throw std::runtime_error(str(format("Failed to open samfile %1%") % path_));

    if (!in_->x.bam)
        throw std::runtime_error(str(format("%1% is not a valid bam file") % path_));

    if (!index_)
        throw std::runtime_error(str(format("Failed to load bam index for %1%") % path_));

    header_.reset(new BamHeader(in_->header));
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

void BamReader::clear_region() {
    if (iter_)
        bam_iter_destroy(iter_);
    iter_ = 0;
}

void BamReader::set_sequence_idx(int32_t tid) {
    tid_ = tid;
    clear_region();
    iter_ = bam_iter_query(index_, tid_, 0, header().seq_length(tid));
}

void BamReader::clear_counts() {
    total_ = 0;
    filtered_ = 0;
}

int BamReader::raw_next(BamEntry& entry) {
    if (iter_)
        return bam_iter_read(in_->x.bam, iter_, entry);
    else
        return bam_read1(in_->x.bam, entry);
}

bool BamReader::next(BamEntry& entry) {
    int rv;
    while ((rv = raw_next(entry)) > 0) {
        ++total_;
        if (!filter_ || filter_->want_entry(entry))
            break;
        ++filtered_;
    }

    // From the samtools source code:
    // (for bam_iter_read)
    // rv >= 0 -> success
    // rv == -1: normal eof
    // rv < -1: error
    if (rv < -1) {
        throw std::runtime_error(str(format(
            "Error while reading bam file %1% (bam_read1 returned %2%; "
            "probably a truncated file)."
            ) % path() % rv));
    }
    return rv >= 0;
}

BamHeader const& BamReader::header() const {
    assert(header_);
    return *header_;
}
