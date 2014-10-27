#include "BamWindow.hpp"
#include "Options.hpp"

#include <iostream>

int main(int argc, char** argv) {
    try {
        Options opts(argc, argv);
        BamWindow app(opts);
        app.exec();
    }
    catch (CmdlineHelpException const& e) {
        std::cout << e.what() << "\n";
        return 0;
    }
    catch (CmdlineError const& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
    catch (std::exception const& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}


