#include <iostream>
#include <cstdlib>
#include "FauxTerm.h"
#include <string>
#include <unistd.h>

using namespace std;

int vim_open(const string &filename, unsigned line, unsigned col) {
    string cmd = "vim \"+call cursor(" + to_string(line) + "," + to_string(col)
        + ")\" " + filename;
    return system(cmd.c_str());
}

#ifdef TEST_VIM_OPEN
static void usage(const string &prog) {
    cerr << "usage: " << prog << " filename line col\n";
}

int main(int argc, char **argv) {
    if (argc != 4) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    const string filename = argv[1];
    unsigned line, col;

    try {
        line = stoul(argv[2]);
        col = stoul(argv[3]);
    } catch (exception &e) {
        cerr << e.what() << "\n";
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    return vim_open(filename, line, col);
}
#endif
