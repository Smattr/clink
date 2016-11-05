#include "CXXParser.h"
#include "Database.h"
#include <iostream>
#include "Symbol.h"
#include <unistd.h>

using namespace std;

class Printer : public SymbolConsumer {
    void consume (const Symbol &s) override {
        cout << s.m_name << " in " << s.m_path << "\n";
    }
};

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

    Printer printer;
    parser.process(printer);

    return EXIT_SUCCESS;
}
