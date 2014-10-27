#pragma once

#include "Options.hpp"

#include <cstdint>
#include <fstream>
#include <memory>
#include <vector>

class BamEntry;
class BamReader;

class BamWindow {
public:
    BamWindow(Options const& opts);

    void exec();

protected:
    bool configure_downsampling() const;
    void open_output_file();

private:
    Options const& opts_;

    // optional output file ptr
    std::unique_ptr<std::ofstream> output_file_ptr_;

    // will point to output_file_ptr_ if -o is given, std::cout otherwise
    std::ostream* out_ptr_;
};
