#include <assert.h>
#include <clang-c/Index.h>
#include <clink/parse_c.h>
#include <clink/symbol.h>
#include <errno.h>
#include "error.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct visit_state {
  int (*callback)(const struct clink_symbol *symbol, void *state);
  void *state;
  const char *parent;
  int error;
};

static enum CXChildVisitResult visit(
    CXCursor cursor,
    CXCursor ignored __attribute__((unused)),
    CXClientData data) {

  // retrieve the type of this symbol
  enum CXCursorKind kind = clang_getCursorKind(cursor);
  enum clink_category category;
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
      category = CLINK_DEFINITION;
      break;
    case CXCursor_CallExpr:
    case CXCursor_MacroExpansion:
      category = CLINK_FUNCTION_CALL;
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
      category = CLINK_REFERENCE;
      break;
    case CXCursor_InclusionDirective:
      category = CLINK_INCLUDE;
      break;

    default: // something irrelevant
      return CXChildVisit_Recurse;
  }

  // retrieve the name of this thing
  CXString ctext = clang_getCursorSpelling(cursor);
  const char *text = clang_getCString(ctext);

  enum CXChildVisitResult rc = CXChildVisit_Recurse;

  // skip entities with no name
  if (strcmp(text, "") == 0)
    goto done;

  struct visit_state *st = (struct visit_state*)data;

  // retrieve its location
  {
    CXSourceLocation loc = clang_getCursorLocation(cursor);
    unsigned lineno, colno;
    CXFile file = NULL;
    clang_getSpellingLocation(loc, &file, &lineno, &colno, NULL);

    // skip if we do not have a meaningful location for this symbol
    if (file == NULL)
      goto done;

    CXString cfilename = clang_getFileName(file);
    const char *filename = clang_getCString(cfilename);

    // tell the caller about this symbol
    const struct clink_symbol s = {
      .category = category,
      .name = (char*)text,
      .name_len = strlen(text),
      .path = (char*)filename,
      .lineno = (unsigned long)lineno,
      .colno = (unsigned long)colno,
      .parent = (char*)st->parent,
    };
    st->error = st->callback(&s, st->state);

    clang_disposeString(cfilename);

    if (st->error != 0) {
      rc = CXChildVisit_Break;
      goto done;
    }
  }

  if (category == CLINK_DEFINITION) {
    // this is something that can serve as the parent to other things

    // duplicate the state, but with this symbol as the parent
    struct visit_state container = *st;
    container.parent = text;

    // visit child nodes of this definition
    bool stop = clang_visitChildren(cursor, visit, &container) != 0;

    // propagate any error that was triggered
    st->error = container.error;

    rc = stop ? CXChildVisit_Break : CXChildVisit_Continue;
  }

done:
  clang_disposeString(ctext);

  return rc;
}

int clink_parse_c(
    const char *filename,
    const char **clang_argv,
    size_t clang_argc,
    int (*callback)(const struct clink_symbol *symbol, void *state),
    void *state) {

  assert(filename != NULL);
  assert(clang_argc == 0 || clang_argv != NULL);
  assert(callback != NULL);

  // check the file exists
  struct stat ignored;
  if (stat(filename, &ignored) != 0)
    return errno;

  // check we can open it for reading
  if (access(filename, R_OK) != 0)
    return errno;

  // create a Clang index
  static const int excludePCH = 0;
  static const int displayDiagnostics = 0;
  CXIndex index = clang_createIndex(excludePCH, displayDiagnostics);

  int rc = 0;

  // parse the file
  CXTranslationUnit tu;
  enum CXErrorCode err = clang_parseTranslationUnit2(
    index, filename, clang_argv, clang_argc, NULL, 0,
    CXTranslationUnit_DetailedPreprocessingRecord|CXTranslationUnit_KeepGoing,
    &tu);
  if (err != CXError_Success) {
    rc = clang_error(err);
    goto fail1;
  }

  // get a top level cursor
  CXCursor cursor = clang_getTranslationUnitCursor(tu);

  // traverse the file
  struct visit_state st = {
    .callback = callback,
    .state = state,
    .parent = NULL,
    .error = 0,
  };
  (void)clang_visitChildren(cursor, visit, &st);

  // retrieve any error the traversal yielded
  rc = st.error;

  clang_disposeTranslationUnit(tu);
fail1:
  clang_disposeIndex(index);

  return rc;
}
