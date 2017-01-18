#pragma once

#include <cstdio>
#include <string>
#include "Parser.h"

enum AsmTokenCategory {
    ASM_NEWLINE,
    ASM_WHITESPACE,
    ASM_EOF,
    ASM_IDENTIFIER,
    ASM_STRING,
    ASM_OTHER,
};

struct AsmToken {
    AsmTokenCategory category;
    std::string text;
};

class AsmLexer {

public:
    AsmLexer();
    ~AsmLexer();
    bool load(const char *path);
    void unload();
    AsmToken next();

private:
    FILE *m_file;

    enum {
        IDLE,
        HASH,
        INCLUDE,
        IGNORING,
    } state;
};

class AsmParser : public Parser {

public:
    virtual ~AsmParser() {}

    bool load(const char *path);
    void unload();

    void process(SymbolConsumer &consumer) override;

private:
    AsmLexer lexer;
    std::string filename;

    FILE *file;

    std::string last_line_text;
    unsigned last_line_number;

    const char *get_context(unsigned line);
};
