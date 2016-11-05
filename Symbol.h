#pragma once

typedef enum {
    ST_DEFINITION,
    ST_FUNCTION_CALL,
    ST_REFERENCE,
    ST_INCLUDE,

    ST_RESERVED,
} symbol_category_t;

struct Symbol {
    symbol_category_t m_category;
    const char *m_name;
    const char *m_path;
    unsigned lineno;
    unsigned col;
};

class SymbolConsumer {

public:
    virtual void consume(const Symbol &s) = 0;

};
