#include <string>
#include "Symbol.h"

Symbol::Symbol(const char *name, const char *path, symbol_category_t category,
        unsigned line, unsigned col, const char *parent, const char *context)
        : m_name(name), m_path(path), m_category(category), m_line(line),
        m_col(col), m_parent(parent ? parent : ""), m_context(context) {
}
