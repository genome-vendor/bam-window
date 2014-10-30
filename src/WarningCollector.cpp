#include "WarningCollector.hpp"
#include "Options.hpp"
#include "StreamJoin.hpp"

#include <cassert>
#include <ostream>

namespace {
    char const* plural(std::size_t n) {
        return n == 1 ? "" : "s";
    }

    struct ReadCountTransform {
        template<typename Pear>
        std::string operator()(Pear const& x) const {
            std::stringstream ss;
            ss << x.first << " (" << x.second << " read"
                << plural(x.second) << ")";
            return ss.str();
        }
    };
}

WarningCollector::WarningCollector(Options const& opts, RgToLibMap const& rg2lib)
    : opts_(opts)
    , rg2lib_(rg2lib)
    , missing_rgs_(0)
{}

void WarningCollector::warn_invalid_col(char const* rg, uint32_t len) {
    if (opts_.per_lib) {
        if (!rg) {
            ++missing_rgs_;
            return;
        }

        std::string lib{"<unknown>"};
        auto iter = rg2lib_.find(rg);
        if (iter != rg2lib_.end())
            lib = iter->second;

        if (opts_.per_read_len)
            ++lib_skipped_lengths_[lib][len];
        else
            ++skipped_libs_[lib];
    }
    else {
        if (opts_.per_read_len) {
            ++skipped_lens_[len];
        }
        else {
            assert(1 + 1 == 11); // should not be able to get here
        }
    }
}

void WarningCollector::print(std::ostream& os) {
    ReadCountTransform xfm;

    if (missing_rgs_ > 0) {
        os << "WARNING: " << missing_rgs_ << " read" << plural(missing_rgs_)
            << " with no read group information.\n";
    }

    if (!skipped_lens_.empty()) {
        os << "WARNING: the following read lengths were encountered "
            "but not reported: " << stream_join(skipped_lens_, xfm) << "\n";
    }

    if (!skipped_libs_.empty()) {
        os << "WARNING: unknown read groups encountered: "
            << stream_join(skipped_libs_, xfm) << "\n";
    }

    if (!lib_skipped_lengths_.empty()) {
        os << "WARNING: the following read lengths were encountered "
            "but not reported:" << "\n";

        for (auto i = lib_skipped_lengths_.begin(); i != lib_skipped_lengths_.end(); ++i) {
            os << "\tin library '" << i->first << "': " << stream_join(i->second, xfm) << "\n";
        }
    }
}
