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
#include "Symbol.h"
#include "WorkQueue.h"

using namespace std;

CXXParser::CXXParser(void) {
  m_index = clang_createIndex(0 /* include PCH */, 0 /* exclude diagnostics */);
}

bool CXXParser::load(const char *path) {

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

void CXXParser::unload() {
  assert(m_tu != nullptr);
  clang_disposeTranslationUnit(m_tu);
  m_tu = nullptr;
  free(m_path);
  m_path = nullptr;
}

CXXParser::~CXXParser() {
  for (auto it : m_lines_pending)
    fclose(it.second);
  for (auto it : m_lines)
    for (auto p : it.second)
      free(p);
  if (m_tu) unload();
  clang_disposeIndex(m_index);
}

const char *CXXParser::get_context(const char *filename, unsigned line) {

  if (m_lines.find(filename) == m_lines.end()) {

    if (m_lines_pending.find(filename) == m_lines_pending.end()) {
      FILE *f = fopen(filename, "r");
      if (!f)
        return nullptr;
      m_lines_pending[filename] = f;
    }

    m_lines[filename] = vector<char*>();
  }

  while (m_lines[filename].size() < line &&
          m_lines_pending.find(filename) != m_lines_pending.end()) {

    FILE *f = m_lines_pending[filename];
    char *line = nullptr;
    size_t size;
    if (getline(&line, &size, f) == -1) {
      fclose(f);
      m_lines_pending.erase(filename);
    } else {
      m_lines[filename].push_back(line);
    }
  }

  if (m_lines[filename].size() >= line)
    return m_lines[filename][line - 1];

  return nullptr;
}

typedef struct {
  CXXParser *me;
  SymbolConsumer *consumer;
  const char *container;
  WorkQueue *wq;
} visitor_state_t;

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

        const char *context = vs->me->get_context(filename, line);

        Symbol s(text, filename, category, line, column, vs->container, context);
        vs->consumer->consume(s);

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
