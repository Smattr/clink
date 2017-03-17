#pragma once

#include <string>

typedef enum {
    ST_DEFINITION,
    ST_FUNCTION_CALL,
    ST_REFERENCE,
    ST_INCLUDE,

    ST_RESERVED,
} symbol_category_t;

class SymbolCore {

 public:
  SymbolCore(const char *name, const char *path, symbol_category_t category,
    unsigned line, unsigned col, const char *parent)
    : m_name(name), m_path(path), m_category(category), m_line(line),
      m_col(col), m_parent(parent ? parent : "") {
  }

  SymbolCore(const std::string &name, std::string &path,
    symbol_category_t category, unsigned line, unsigned col, const char *parent)
    : m_name(name), m_path(path), m_category(category), m_line(line),
      m_col(col), m_parent(parent ? parent : "") {
  }

  const char *name() const { return m_name.c_str(); }
  const char *path() const { return m_path.c_str(); }
  symbol_category_t category() const { return m_category; }
  unsigned line() const { return m_line; }
  unsigned col() const { return m_col; }
  const char *parent() const {
    return m_parent == "" ? "<global>" : m_parent.c_str();
  }

  virtual ~SymbolCore() {}

 protected:
  std::string m_name;
  std::string m_path;
  symbol_category_t m_category;
  unsigned m_line;
  unsigned m_col;
  std::string m_parent;

};

class Symbol : public SymbolCore {

 public:
  Symbol(const char *name, const char *path, symbol_category_t category,
    unsigned line, unsigned col, const char *parent, const char *context)
    : SymbolCore(name, path, category, line, col, parent), m_context(context) {
  }

  Symbol(const std::string &name, std::string &path, symbol_category_t category,
    unsigned line, unsigned col, const char *parent, const char *context)
    : SymbolCore(name, path, category, line, col, parent), m_context(context) {
  }

  const char *context() const { return m_context.c_str(); }

  virtual ~Symbol() {}

 private:
  std::string m_context;

};

class SymbolConsumer {

public:
    virtual void consume(const Symbol &s) = 0;
    virtual bool purge(const std::string &path) = 0;

    virtual ~SymbolConsumer() {}

};
