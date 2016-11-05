#include "CPPParser.h"
#include <iostream>
#include "Symbol.h"

using namespace std;

class Printer : public SymbolConsumer {
    void consume (const Symbol &s) override {
        cout << s.m_name << " in " << s.m_path << "\n";
    }
};

int main() {
    CPPParser parser;

    if (!parser.load("foo.cpp")) {
        cerr << "failed to load foo.cpp\n";
        return -1;
    }

    Printer printer;
    parser.process(printer);

    return 0;
}
