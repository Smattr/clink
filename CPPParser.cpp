/* XXX: Commit some momentary lunacy here. We need to forward-declare visitor to
 * to guarantee it gets both friend and static linkage.
 */
#include <clang-c/Index.h>
static CXChildVisitResult visitor(CXCursor cursor, CXCursor /* ignored */,
    CXClientData data);

#include <assert.h>
#include "CPPParser.h"
#include <iostream>
#include <string.h>
#include "Symbol.h"

using namespace std;

CPPParser::CPPParser(void)
        : m_tu(nullptr), m_path(nullptr) {
    m_index = clang_createIndex(0 /* include PCH */,
        0 /* exclude diagnostics */);
}

bool CPPParser::load(const char *path) {

    // Unload any currently loaded file.
    if (m_tu) unload();

    m_path = strdup(path);
    if (!m_path)
        return false;

    // Load and parse the file.
    m_tu = clang_parseTranslationUnit(m_index, path, nullptr, 0, nullptr, 0,
        CXTranslationUnit_DetailedPreprocessingRecord|CXTranslationUnit_KeepGoing);

    return m_tu != nullptr;
}

void CPPParser::unload() {
    assert(m_tu != nullptr);
    clang_disposeTranslationUnit(m_tu);
    m_tu = nullptr;
    free(m_path);
    m_path = nullptr;
}

CPPParser::~CPPParser() {
    if (m_tu) unload();
    clang_disposeIndex(m_index);
}

// Clang visitor. Herein is the core logic of the parser.
static CXChildVisitResult visitor(CXCursor cursor, CXCursor /* ignored */,
        CXClientData data) {

    /* Retrieve the type of this symbol. Depending on what it is, it may not be
     * relevant to report to the callback.
     */
    CXCursorKind kind = clang_getCursorKind(cursor);
    symbol_category_t category;
    switch (kind) {
        case CXCursor_StructDecl:
        case CXCursor_UnionDecl:
        case CXCursor_ClassDecl:
        case CXCursor_EnumDecl:
        case CXCursor_FieldDecl:
        case CXCursor_EnumConstantDecl:
        case CXCursor_FunctionDecl:
        case CXCursor_VarDecl:
        case CXCursor_ParmDecl:
        case CXCursor_TypedefDecl:
        case CXCursor_CXXMethod:
        case CXCursor_Namespace:
        case CXCursor_Constructor:
        case CXCursor_Destructor:
        case CXCursor_ConversionFunction:
        case CXCursor_TemplateTypeParameter:
        case CXCursor_NonTypeTemplateParameter:
        case CXCursor_TemplateTemplateParameter:
        case CXCursor_FunctionTemplate:
        case CXCursor_ClassTemplate:
        case CXCursor_ClassTemplatePartialSpecialization:
        case CXCursor_NamespaceAlias:
        case CXCursor_TypeAliasDecl:
        case CXCursor_MacroDefinition:
            category = ST_DEFINITION;
            break;
        case CXCursor_CallExpr:
            category = ST_FUNCTION_CALL;
            break;
        case CXCursor_UsingDirective:
        case CXCursor_UsingDeclaration:
        case CXCursor_TypeRef:
        case CXCursor_TemplateRef:
        case CXCursor_NamespaceRef:
        case CXCursor_MemberRef:
        case CXCursor_LabelRef:
        case CXCursor_OverloadedDeclRef:
        case CXCursor_VariableRef:
        case CXCursor_DeclRefExpr:
        case CXCursor_MemberRefExpr:
        case CXCursor_CXXThisExpr:
            category = ST_REFERENCE;
            break;
        case CXCursor_InclusionDirective:
            category = ST_INCLUDE;
            break;
        default:
            category = ST_RESERVED;
    };

    if (kind != ST_RESERVED) {

        /* Retrieve the name of this entity. */
        CXString cxtext = clang_getCursorSpelling(cursor);
        const char *text = clang_getCString(cxtext);

        if (strcmp(text, "")) {

            /* Retrieve its location. */
            CXSourceLocation loc = clang_getCursorLocation(cursor);
            unsigned line, column;
            CXFile file;
            clang_getSpellingLocation(loc, &file, &line, &column, nullptr);

            if (file != nullptr) {

                CXString cxfilename = clang_getFileName(file);
                const char *filename = clang_getCString(cxfilename);

                SymbolConsumer *consumer = (SymbolConsumer*)data;
                Symbol s {category, text, filename, line, column};
                consumer->consume(s);

                clang_disposeString(cxfilename);
            }

        }
        clang_disposeString(cxtext);

    }

    clang_visitChildren(cursor, visitor, data);
    return CXChildVisit_Continue;
}

void CPPParser::process(SymbolConsumer &consumer) {
    assert(m_tu != nullptr);
    CXCursor cursor = clang_getTranslationUnitCursor(m_tu);
    clang_visitChildren(cursor, visitor, &consumer);
}
