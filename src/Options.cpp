#include "Options.hpp"
#include "version.h"

#include <bam.h>

#include <boost/format.hpp>

#include <ios>
#include <iomanip>
#include <sstream>

namespace po = boost::program_options;
using boost::format;

#ifndef BAM_FSUPPLEMENTAL
# define BAM_FSUPPLEMENTAL 2048
#endif

namespace {
    // From the SAM spec
    std::vector<std::pair<int, char const*>> SAM_FLAG_DESCRIPTIONS = {
          {  0x1, "template having multiple segments in sequencing"}
        , {  0x2, "each segment properly aligned according to the aligner"}
        , {  0x4, "segment unmapped"}
        , {  0x8, "next segment in the template unmapped"}
        , { 0x10, "SEQ being reverse complemented"}
        , { 0x20, "SEQ of the next segment in the template being reversed"}
        , { 0x40, "the first segment in the template"}
        , { 0x80, "the last segment in the template"}
        , {0x100, "secondary alignment"}
        , {0x200, "not passing quality controls"}
        , {0x400, "PCR or optical duplicate"}
        , {0x800, "supplementary alignment"}
        };

    std::string sam_flags_str() {
        std::stringstream ss;

        auto const& x = SAM_FLAG_DESCRIPTIONS;
        std::string divider(78, '-');

        ss << std::setw(8) << "Decimal" << " "
            << std::setw(8) << "Hex" << "  " << "Description" << "\n"
            << divider << "\n";

        for (auto i = x.begin(); i != x.end(); ++i) {
            ss << std::setw(8) << std::dec << i->first << " "
                << std::setw(8) << std::showbase << std::hex << i->first
                << "  " << i->second << "\n";
        }

        return ss.str();
    }
}


std::string Options::help_message() const {
    std::stringstream ss;
    ss << "\nUsage: " << program_name << " [OPTIONS]" << " <input-file>\n\n";
    ss << opts << "\n";
    return ss.str();
}

std::string Options::version_message() const {
    std::stringstream ss;
    ss << "\nExecutable:\t" << program_name << "\n"
        << "Version info:\t" << _g_git_version_info << "\n"
        << "Build type:\t" << _g_build_type << "\n"
        ;

    return ss.str();
}

void Options::check_help() const {
    if (var_map.count("list-sam-flags")) {
        throw CmdlineHelpException(sam_flags_str());
    }

    if (var_map.count("version")) {
        throw CmdlineHelpException(version_message());
    }

    if (var_map.count("help")) {
        throw CmdlineHelpException(help_message());
    }
}

Options::Options(int argc, char** argv)
    : program_name(argv[0])
{
    pos_opts.add("input-file", 1);

    po::options_description help_opts("Help Options");
    help_opts.add_options()
        ("help,h", "this message")
        ("version,v", "display version information")
        ("list-sam-flags", "Print SAM flag reference table")
        ;

    po::options_description gen_opts("General Options");
    gen_opts.add_options()
        ("input-file,i"
            , po::value<std::string>(&input_file)->required()
            , "Sorted, indexed bam file to count reads in (1st positional arg)")

        ("output-file,o"
            , po::value<std::string>(&output_file)->default_value("-")
            , "Output file (- for stdout)")

        ("sequence,c"
            , po::value<std::vector<std::string>>(&sequence_names)
            , "Sequence/chromosome name to operate on (may be specified "
              "multiple times). By default, all sequences are processed")
        ;

    po::options_description rep_opts("Reporting Options");
    rep_opts.add_options()
        ("window-size,w"
            , po::value<int>(&window_size)->default_value(1000)
            , "Tiling window size")

        ("leftmost,s"
            , po::bool_switch(&leftmost)->default_value(false)
            , "Use only the leftmost position of each read "
              "(i.e., don't let reads span windows)")

        ("by-library,l"
            , po::bool_switch(&per_lib)->default_value(false)
            , "Count and report reads (in columns) per-library")

        ("by-read-length,r"
            , po::bool_switch(&per_read_len)->default_value(false)
            , "Count and report reads (in columns) per-read length "
              "(compatible with -l)")
        ;

    po::options_description flt_opts("Filtering Options");
    flt_opts.add_options()
        ("min-mapq,q"
            , po::value<int>(&min_mapq)->default_value(0)
            , "Filter reads with mapping quality less than this")

        ("pairs-only,p"
            , po::bool_switch(&pairs_only)->default_value(false)
            , "Only include paired-end reads (equivalent to -f 1)")

        ("proper-pairs-only,P"
            , po::bool_switch(&proper_pairs_only)->default_value(false)
            , "Only include 'properly' paired reads (equivalent to -f 2)")

       ("downsample,d"
            , po::value<float>(&downsample)->default_value(1.0f)
            , "If set to something < 1.0, report reads with this probability")

        ("seed,S"
            , po::value<std::string>(&seed_string)->default_value("")
            , "Seed for random number generator when downsampling. "
              "The current time is used by default.")

        ("required-flags,f"
            , po::value<int>(&required_flags)->default_value(0)
            , "SAM flags that each read must have")

        ("forbidden-flags,F"
            , po::value<int>(&forbidden_flags)->default_value(
                BAM_FSECONDARY | BAM_FSUPPLEMENTAL
                )
            , "SAM flags that each read is forbidden to have")
        ;

    opts.add(help_opts).add(gen_opts).add(rep_opts).add(flt_opts);

    if (argc <= 1)
        throw CmdlineHelpException(help_message());

    try {

        auto parsed_opts = po::command_line_parser(argc, argv)
                .options(opts)
                .positional(pos_opts).run();

        po::store(parsed_opts, var_map);
        po::notify(var_map);
        validate();

    } catch (std::exception const& e) {
        // program options will throw if required options are not passed
        // before we have a chance to check if the user has asked for
        // --help. If they have, let's give it to them, otherwise, rethrow.
        check_help();

        std::stringstream ss;
        ss << help_message() << "\n\nERROR: " << e.what() << "\n";
        throw CmdlineError(ss.str());
    }

    check_help();

}

void Options::validate() {
    if (pairs_only)
        required_flags |= BAM_FPAIRED;

    if (proper_pairs_only)
        required_flags |= BAM_FPROPER_PAIR | BAM_FPAIRED;

    if ((required_flags & forbidden_flags) != 0) {
        throw std::runtime_error(str(format(
            "Required flags (%1%) and forbidden flags (%2%) must be disjoint."
            ) % required_flags % forbidden_flags));
    }

    if (window_size < 1) {
        throw std::runtime_error(str(format(
            "Invalid window size (%1%), must be >= 1."
            ) % window_size));
    }

    if (downsample <= 0.0f || downsample > 1.0f) {
        throw std::runtime_error(str(format(
            "Invalid downsampling value (%1%), must be > 0 and <= 1."
            ) % downsample));
    }

    if (!seed_string.empty()) {
        try {
            seed = boost::lexical_cast<long>(seed_string);
        }
        catch (...) {
            throw std::runtime_error(str(format(
                "Invalid seed value '%1%', argument must be numeric."
                ) % seed_string));
        }
    }
}
