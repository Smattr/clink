#include <cstdlib>
#include "FauxTerm.h"
#include <string>

using namespace std;

int vim_open(const string &filename, unsigned line, unsigned col) {
    string cmd = "vim \"+call cursor(" + to_string(line) + "," + to_string(col)
        + ")\" " + filename;
    return system(cmd.c_str());
}
