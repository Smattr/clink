#include "CXXParser.h"
#include "Database.h"
#include <iostream>
#include "Symbol.h"
#include <unistd.h>

using namespace std;

static const char default_database[] = ".clink.db";

int main() {
    Database db;
    if (!db.open(default_database)) {
        cerr << "failed to open " << default_database << "\n";
        return EXIT_FAILURE;
    }

    CPPParser parser;

    if (!parser.load("foo.cpp")) {
        cerr << "failed to load foo.cpp\n";
        return EXIT_FAILURE;
    }

    parser.process(db);

    return EXIT_SUCCESS;
}
