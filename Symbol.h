#pragma once

typedef enum {
    ST_DEFINITION,
    ST_FUNCTION_CALL,
    ST_REFERENCE,
    ST_INCLUDE,

    ST_RESERVED,
} symbol_category_t;

struct Symbol {
    const char *name;
    const char *path;
    symbol_category_t category;
    unsigned line;
    unsigned col;
    const char *parent;
};

class SymbolConsumer {

public:
    virtual void consume(const Symbol &s) = 0;

};
