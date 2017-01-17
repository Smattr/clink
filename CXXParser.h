#pragma once

#include <clang-c/Index.h>
#include "Parser.h"
#include "Symbol.h"
#include <unordered_map>
#include <vector>

class CXXParser : public Parser {

public:

    CXXParser();
    virtual ~CXXParser();

    /* Load a new file. Automatically unloads any currently loaded file. Returns
     * true on success.
     */
    bool load(const char *path);

    /* Unload the currently loaded file. This function assumes there is a
     * currently loaded file.
     */
    void unload();

    /* Process the currently loaded file. That is, extract all relevant content.
     * This function assumes there is a currently loaded file. You are expected
     * to retrieve the extracted data following this using next().
     */
    void process(SymbolConsumer &consumer) override;

private:
    const char *get_context(const char *filename, unsigned line);

private:
    // Internal handle to Clang.
    CXIndex m_index;

    /* The current translation unit. This gets reassigned whenever we load a new
     * file. NULL when no file is currently loaded.
     */
    CXTranslationUnit m_tu;

    char *m_path;

    std::unordered_map<std::string, std::vector<char*>> m_lines;
    std::unordered_map<std::string, FILE*> m_lines_pending;

    friend CXChildVisitResult visitor(CXCursor cursor, CXCursor /* ignored */,
        CXClientData data);
};
