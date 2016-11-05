#pragma once

#include <clang-c/Index.h>
#include "Parser.h"
#include "Symbol.h"

class CPPParser : public Parser {

public:

    CPPParser();
    ~CPPParser();

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

protected:
    // Internal handle to Clang.
    CXIndex m_index;

    /* The current translation unit. This gets reassigned whenever we load a new
     * file. NULL when no file is currently loaded.
     */
    CXTranslationUnit m_tu;

    char *m_path;

    friend CXChildVisitResult visitor(CXCursor cursor, CXCursor /* ignored */,
        CXClientData data);
};
