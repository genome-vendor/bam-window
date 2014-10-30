#pragma once

#include <ostream>
#include <sstream>
#include <string>

struct Identity {
    template<typename T>
    T const& operator()(T const& x) const {
        return x;
    }
};

template<typename Iterable, typename Transform>
struct StreamJoin {

    explicit StreamJoin(Iterable const& seq, Transform xfm = Transform())
        : seq(seq)
        , empty(".")
        , delim(",")
        , xfm(xfm)
    {
    }

    StreamJoin& emptyString(char const* empty) {
        this->empty = empty;
        return *this;
    }

    StreamJoin& delimiter(char const* delim) {
        this->delim = delim;
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& out, StreamJoin const& sj) {
        if (sj.seq.empty()) {
            out << sj.empty;
        }
        else {
            auto i = sj.seq.begin();
            out << sj.xfm(*i);
            for (++i; i != sj.seq.end(); ++i) {
                out << sj.delim << sj.xfm(*i);
            }
        }
        return out;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }

    Iterable const& seq;
    char const* empty;
    char const* delim;
    Transform xfm;
};

template<typename Iterable, typename Transform = Identity>
StreamJoin<Iterable, Transform>
stream_join(Iterable const& seq, Transform transform = Transform()) {
    return StreamJoin<Iterable, Transform>(seq);
}
