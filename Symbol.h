#pragma once

#include <string>

typedef enum {
    ST_DEFINITION,
    ST_FUNCTION_CALL,
    ST_REFERENCE,
    ST_INCLUDE,

    ST_RESERVED,
} symbol_category_t;

class Symbol {

public:
    Symbol(const char *name, const char *path, symbol_category_t category,
        unsigned line, unsigned col, const char *parent, const char *context);

    const char *name() const { return m_name.c_str(); }
    const char *path() const { return m_path.c_str(); }
    symbol_category_t category() const { return m_category; }
    unsigned line() const { return m_line; }
    unsigned col() const { return m_col; }
    const char *parent() const {
        return m_parent == "" ? "<global>" : m_parent.c_str();
    }
    const char *context() const { return m_context.c_str(); }

private:
    std::string m_name;
    std::string m_path;
    symbol_category_t m_category;
    unsigned m_line;
    unsigned m_col;
    std::string m_parent;
    std::string m_context;

};

class SymbolConsumer {

public:
    virtual void consume(const Symbol &s) = 0;

};
