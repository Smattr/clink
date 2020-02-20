/* XXX: Commit some momentary lunacy here. We need to forward-declare visitor to
 * to guarantee it gets both friend and static linkage.
 */
#include <clang-c/Index.h>
static CXChildVisitResult visitor(CXCursor cursor, CXCursor /* ignored */,
    CXClientData data);

#include <cassert>
#include <cstring>
#include "CXXParser.h"
#include <iostream>
#include "log.h"
#include "Options.h"
#include "Symbol.h"
#include "WorkQueue.h"

using namespace std;

CXXParser::CXXParser(void) {
  m_index = clang_createIndex(0 /* include PCH */, 0 /* exclude diagnostics */);
}

bool CXXParser::load(const char *path) {

  // Unload any currently loaded file.
  if (m_tu) unload();

  // Construct command-line arguments
  int argc = opts.include_dirs.size() * 2;
  const char **argv = new const char*[argc];
  for (int i = 0; i < argc; i += 2) {
    argv[i] = "-I";
    argv[i + 1] = opts.include_dirs[i / 2].c_str();
  }

  // Load and parse the file.
  m_tu = clang_parseTranslationUnit(m_index, path, argv, argc, nullptr, 0,
    CXTranslationUnit_DetailedPreprocessingRecord|CXTranslationUnit_KeepGoing);

  delete[] argv;

  return m_tu != nullptr;
}

void CXXParser::unload() {
  assert(m_tu != nullptr);
  clang_disposeTranslationUnit(m_tu);
  m_tu = nullptr;
}

CXXParser::~CXXParser() {
  if (m_tu) unload();
  clang_disposeIndex(m_index);
}

typedef struct {
  CXXParser *me;
  SymbolConsumer *consumer;
  const char *container;
  WorkQueue *wq;
} visitor_state_t;

// Debugging method
static void log_symbol_ignore(CXCursor cursor) {
  assert(log_file != nullptr);

  // Get the text of the symbol itself.
  CXString cxtext = clang_getCursorSpelling(cursor);
  const char *text = clang_getCString(cxtext);

  // If the symbol text is unavailable, don't bother logging this.
  if (strcmp(text, "") == 0) {
    clang_disposeString(cxtext);
    return;
  }

  // Figure out what type of thing it is.
  CXCursorKind kind = clang_getCursorKind(cursor);
  CXString cxkind = clang_getCursorKindSpelling(kind);
  const char *kindstr = clang_getCString(cxkind);

  // Find the file we're looking at.
  CXSourceLocation loc = clang_getCursorLocation(cursor);
  unsigned line, column;
  CXFile file;
  clang_getSpellingLocation(loc, &file, &line, &column, nullptr);

  if (file != nullptr) {
    CXString cxfilename = clang_getFileName(file);
    const char *filename = clang_getCString(cxfilename);
    LOG("ignoring %s symbol '%s' from %s:%u:%u", kindstr, text, filename, line,
      column);
    clang_disposeString(cxfilename);
  } else {
    LOG("ignoring %s symbol '%s'", kindstr, text);
  }

  clang_disposeString(cxkind);
  clang_disposeString(cxtext);
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
    case CXCursor_MacroExpansion:
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
    case CXCursor_UnexposedExpr:
      category = ST_REFERENCE;
      break;
    case CXCursor_InclusionDirective:
      category = ST_INCLUDE;
      break;
    default:
      if (log_file != nullptr)
        log_symbol_ignore(cursor);
      category = ST_RESERVED;
  };

  if (category != ST_RESERVED) {

    /* Retrieve the name of this entity. */
    CXString cxtext = clang_getCursorSpelling(cursor);
    const char *text = clang_getCString(cxtext);

    visitor_state_t *vs = (visitor_state_t*)data;

    if (strcmp(text, "")) {

      /* Retrieve its location. */
      CXSourceLocation loc = clang_getCursorLocation(cursor);
      unsigned line, column;
      CXFile file;
      clang_getSpellingLocation(loc, &file, &line, &column, nullptr);

      if (file != nullptr) {

        CXString cxfilename = clang_getFileName(file);
        const char *filename = clang_getCString(cxfilename);

        SymbolCore s(text, filename, category, line, column, vs->container);
        vs->consumer->consume(s);

        /* Check if we've already queued this file to be read (in which case we
         * don't need to push it into the work queue).
         */
        if (filename != vs->me->last_seen) {
          /* Queue a read of this file, now we know we need its data. Note that
           * someone else may have already requested this file's contents in
           * which case the work queue will dedupe this request.
           */
          vs->wq->push(filename);
          vs->me->last_seen = filename;
        }

        clang_disposeString(cxfilename);
      }

    }

    visitor_state_t for_child = *vs;
    if (category == ST_DEFINITION && strcmp(text, ""))
      for_child.container = text;
    clang_visitChildren(cursor, visitor, &for_child);

    clang_disposeString(cxtext);

  } else {
    clang_visitChildren(cursor, visitor, data);
  }

  return CXChildVisit_Continue;
}

void CXXParser::process(SymbolConsumer &consumer, WorkQueue *wq) {
  assert(m_tu != nullptr);
  CXCursor cursor = clang_getTranslationUnitCursor(m_tu);
  visitor_state_t vs {
    .me = this, .consumer = &consumer, .container = nullptr, .wq = wq
  };
  clang_visitChildren(cursor, visitor, &vs);
}
