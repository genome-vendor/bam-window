#pragma once

#include <boost/program_options.hpp>

#include <stdexcept>
#include <string>
#include <vector>

class CmdlineHelpException : public std::runtime_error {
public:
    CmdlineHelpException(std::string const& msg)
        : std::runtime_error(msg)
    {
    }
};

class CmdlineError : public std::runtime_error {
public:
    CmdlineError(std::string const& msg)
        : std::runtime_error(msg)
    {
    }
};


struct Options {
    std::string program_name;

    std::string input_file;
    std::string output_file;
    int min_mapq;
    int window_size;
    int required_flags;
    int forbidden_flags;
    bool pairs_only;
    bool proper_pairs_only;
    bool leftmost;
    bool per_lib;
    bool per_read_len;
    std::string seed_string;
    long seed;
    float downsample;
    std::vector<std::string> sequence_names;


    Options(int argc, char** argv);


    void validate();

private:
    std::string help_message() const;
    void check_help() const;

    boost::program_options::options_description opts;
    boost::program_options::positional_options_description pos_opts;
    boost::program_options::variables_map var_map;


};
