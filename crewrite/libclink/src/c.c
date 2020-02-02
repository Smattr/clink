#include <assert.h>
#include <clang-c/Index.h>
#include <clink/c.h>
#include <clink/compiler.h>
#include <clink/symbol.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// translate a CXErrorCode to the closest errno
static int cxerror_to_errno(enum CXErrorCode err) {
  switch (err) {
    case CXError_Success:          return 0;
    case CXError_Failure:          return ENOSYS;
    case CXError_Crashed:          return ENOTRECOVERABLE;
    case CXError_InvalidArguments: return EINVAL;
    case CXError_ASTReadError:     return EIO;
  }
}

typedef struct {
  int error;
  char *parent;

  int (*callback)(void *state, const clink_symbol_t *symbol);
  void *callback_state;
} visit_state_t;

static enum CXChildVisitResult visit(
    CXCursor cursor,
    CLINK_UNUSED CXCursor parent,
    CXClientData data) {

  CXString cxtext;
  const char *text = NULL;
  CXString cxfilename;
  const char *filename = NULL;

  // retrieve the type of this symbol
  enum CXCursorKind kind = clang_getCursorKind(cursor);
  clink_category_t category;
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
      category = CLINK_ST_DEFINITION;
      break;
    case CXCursor_CallExpr:
    case CXCursor_MacroExpansion:
      category = CLINK_ST_CALL;
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
      category = CLINK_ST_REFERENCE;
      break;
    case CXCursor_InclusionDirective:
      category = CLINK_ST_INCLUDE;
      break;

    default: // something irrelevant
      goto done;
  }

  // retrieve the name of this thing
  cxtext = clang_getCursorSpelling(cursor);
  text = clang_getCString(cxtext);
  assert(text != NULL);

  if (strcmp(text, "") == 0) {
    // skip entities with no name
    goto done;
  }

  visit_state_t *st = (visit_state_t*)data;
  assert(st != NULL);
  assert(st->callback != NULL);

  // retrieve its location
  {
    CXSourceLocation loc = clang_getCursorLocation(cursor);
    unsigned line, column;
    CXFile file = NULL;
    clang_getSpellingLocation(loc, &file, &line, &column, NULL);

    if (file == NULL) {
      // we do not have a meaningful location for this symbol
      goto done;
    }

    cxfilename = clang_getFileName(file);
    filename = clang_getCString(cxfilename);
    assert(filename != NULL);

    const clink_symbol_t symbol = {
      .category = category,
      .name = text,
      .name_len = strlen(text),
      .path = filename,
      .line = line,
      .column = column,
      .parent = st->parent,
    };

    // tell the caller about this symbol
    if ((st->error = st->callback(st->callback_state, &symbol))) {
      goto done;
    }
  }

  clang_disposeString(cxfilename);
  filename = NULL;

  if (category == CLINK_ST_DEFINITION) {
    // this is something that can serve as the parent to other things
    visit_state_t container = *st;
    container.parent = (char*)text;
    bool stop = clang_visitChildren(cursor, visit, &container) != 0;

    // propagate any error that was triggered
    st->error = container.error;

    // clean up
    clang_disposeString(cxtext);

    // any error detected while traversing our children should have caused
    // visit() to return CXChildVisit_Break
    assert(st->error == 0 || stop);

    return stop ? CXChildVisit_Break : CXChildVisit_Continue;
  }
  // otherwise, fall through to recurse

done:
  if (filename != NULL)
    clang_disposeString(cxfilename);

  if (text != NULL)
    clang_disposeString(cxtext);

  return CXChildVisit_Recurse;
}

int clink_parse_c(
    const char *filename,
    int (*callback)(void *state, const clink_symbol_t *symbol),
    void *state) {

  assert(filename != NULL);
  assert(callback != NULL);

  // create a Clang index
  static const int excludePCH = 0;
  static const int displayDiagnostics = 0;
  CXIndex index = clang_createIndex(excludePCH, displayDiagnostics);

  int rc = 0;

  // parse file
  CXTranslationUnit tu = NULL;
  enum CXErrorCode err
    = clang_parseTranslationUnit2(index, filename, NULL, 0, NULL, 0, 
    CXTranslationUnit_DetailedPreprocessingRecord|CXTranslationUnit_KeepGoing,
    &tu);

  if (err != CXError_Success) {
    rc = cxerror_to_errno(err);
    goto done;
  }

  // find a top level cursor
  CXCursor cursor = clang_getTranslationUnitCursor(tu);

  // traverse the file
  visit_state_t st = { .callback = callback, .callback_state = state };
  (void)clang_visitChildren(cursor, visit, &st);

  // retrieve any error the traversal yielded
  rc = st.error;

done:
  if (tu != NULL)
    clang_disposeTranslationUnit(tu);

  clang_disposeIndex(index);

  return rc;
}

static int print(CLINK_UNUSED void *state, const clink_symbol_t *symbol) {

  const char *category =
    symbol->category == CLINK_ST_DEFINITION ? "definition of" :
    symbol->category == CLINK_ST_INCLUDE ? "include of" :
    symbol->category == CLINK_ST_CALL ? "function call of" : "reference to";

  printf("%s:%lu:%lu: %s %.*s\n", symbol->path, symbol->line, symbol->column,
    category, (int)symbol->name_len, symbol->name);

  return 0;
}

static CLINK_UNUSED int main_c(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "usage: %s filename\n", argv[0]);
    return EXIT_FAILURE;
  }

  int rc = clink_parse_c(argv[1], print, NULL);
  if (rc != 0) {
    fprintf(stderr, "parsing failed: %s\n", strerror(rc));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

#ifdef MAIN_C
int main(int argc, char **argv) {
  return main_c(argc, argv);
}
#endif
