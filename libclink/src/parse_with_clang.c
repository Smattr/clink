#include "debug.h"
#include <assert.h>
#include <clang-c/Index.h>
#include <clink/clang.h>
#include <clink/db.h>
#include <clink/symbol.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static void deinit(CXTranslationUnit tu, CXIndex index) {
  if (tu != NULL)
    clang_disposeTranslationUnit(tu);

  clang_disposeIndex(index);
}

// determine if this cursor can be a semantic parent of something else
static bool is_parent(CXCursor cursor) {
  switch (clang_getCursorKind(cursor)) {
  case CXCursor_StructDecl:
  case CXCursor_UnionDecl:
  case CXCursor_ClassDecl:
  case CXCursor_EnumDecl:
  case CXCursor_FunctionDecl:
  case CXCursor_TypedefDecl:
  case CXCursor_CXXMethod:
  case CXCursor_Namespace:
  case CXCursor_Constructor:
  case CXCursor_Destructor:
  case CXCursor_ConversionFunction:
  case CXCursor_FunctionTemplate:
  case CXCursor_ClassTemplate:
  case CXCursor_ClassTemplatePartialSpecialization:
  case CXCursor_MacroDefinition:
    return true;
  default:
    break;
  }
  return false;
}

// state used by the visitor
typedef struct {

  /// database to insert into
  clink_db_t *db;

  /// named parent of the current context during traversal
  const char *current_parent;

  /// status of our Clang traversal (0 OK, non-zero on error)
  int rc;

} state_t;

static int add_symbol(state_t *state, clink_category_t category,
                      const char *name, const char *path, unsigned lineno,
                      unsigned colno) {

  assert(state != NULL);

  clink_symbol_t symbol = {.category = category,
                           .name = (char *)name,
                           .path = (char *)path,
                           .lineno = lineno,
                           .colno = colno,
                           .parent = (char *)state->current_parent};
  state->rc = clink_db_add_symbol(state->db, &symbol);

  return state->rc;
}

static enum CXChildVisitResult visit(CXCursor cursor, CXCursor parent,
                                     CXClientData data);

static int visit_children(state_t *state, CXCursor cursor) {

  assert(state != NULL);
  assert(state->rc == 0 && "failure did not terminate traversal");

  // default to the parent of the current context
  const char *parent = state->current_parent;

  CXString text = {0};

  // if this node is a definition, it can serve as a semantic parent to children
  if (is_parent(cursor)) {

    // extract its name
    text = clang_getCursorSpelling(cursor);
    const char *ctext = clang_getCString(text);

    // is this a valid name for a parent?
    bool ok_parent = ctext != NULL && strcmp(ctext, "") != 0;

    // save it for use
    if (ok_parent)
      parent = ctext;
  }

  // state for descendants of this cursor to see
  state_t for_children = {.db = state->db, .current_parent = parent};

  // recursively descend into this cursor’s children
  (void)clang_visitChildren(cursor, visit, &for_children);

  // propagate any errors the visitation encountered
  state->rc = for_children.rc;

  if (text.data != NULL)
    clang_disposeString(text);

  return state->rc;
}

static enum CXChildVisitResult visit(CXCursor cursor, CXCursor parent,
                                     CXClientData state) {

  // we do not need the parent cursor
  (void)parent;

  int rc = 0;

  // ignore anything not from the main file
  CXSourceLocation loc = clang_getCursorLocation(cursor);
  if (!clang_Location_isFromMainFile(loc))
    return CXChildVisit_Continue;

  // retrieve the type of this symbol
  enum CXCursorKind kind = clang_getCursorKind(cursor);
  clink_category_t category = CLINK_REFERENCE;
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
  CXString text = clang_getCursorSpelling(cursor);
  const char *name = clang_getCString(text);

  // skip entities with no name
  if (name == NULL || strcmp(name, "") == 0)
    goto done;

  // retrieve its location
  {
    unsigned lineno, colno;
    CXFile file = NULL;
    clang_getSpellingLocation(loc, &file, &lineno, &colno, NULL);

    // skip if we do not have a meaningful location for this symbol
    if (file == NULL)
      goto done;

    CXString filename = clang_getFileName(file);
    const char *fname = clang_getCString(filename);

    // add this symbol to the database
    rc = add_symbol(state, category, name, fname, lineno, colno);

    clang_disposeString(filename);
  }

done:
  clang_disposeString(text);

  // abort visitation if we have seen an error
  if (rc)
    return CXChildVisit_Break;

  // recurse into the children
  rc = visit_children(state, cursor);
  if (rc)
    return CXChildVisit_Break;

  return CXChildVisit_Continue;
}

// translate a libclang error into the closest errno equivalent
static int clang_err_to_errno(int err) {
  switch (err) {
  case CXError_Failure:
    return ENOTRECOVERABLE;
  case CXError_Crashed:
    return EIO;
  case CXError_InvalidArguments:
    return EINVAL;
  case CXError_ASTReadError:
    return EPROTO;
  default:
    return 0;
  }
}

static int init(CXIndex *index, CXTranslationUnit *tu, const char *filename,
                size_t argc, const char **argv) {

  assert(index != NULL);
  assert(tu != NULL);
  assert(filename != NULL);
  assert(argc == 0 || argv != NULL);

  // if the caller did not provide any arguments, fall back on some defaults
  if (argc == 0) {
    static const char *DEFAULT[] = {"clang", NULL};
    argv = DEFAULT;
    argc = sizeof(DEFAULT) / sizeof(DEFAULT[0]) - 1;
  }

  // create a Clang index
  static const int excludePCH = 0;
  static const int displayDiagnostics = 0;
  *index = clang_createIndex(excludePCH, displayDiagnostics);

  // parse the input file
  unsigned options = CXTranslationUnit_None;
  options |= CXTranslationUnit_DetailedPreprocessingRecord;
  options |= CXTranslationUnit_KeepGoing;
  options |= CXTranslationUnit_SkipFunctionBodies;
  options |= CXTranslationUnit_LimitSkipFunctionBodiesToPreamble;
#if CINDEX_VERSION_MINOR >= 60
  options |= CXTranslationUnit_RetainExcludedConditionalBlocks;
#endif
  enum CXErrorCode err = clang_parseTranslationUnit2(
      *index, filename, argv, (int)argc, NULL, 0, options, tu);
  if (ERROR(err != CXError_Success))
    return clang_err_to_errno(err);

  return 0;
}

int clink_parse_with_clang(clink_db_t *db, const char *filename, size_t argc,
                           const char **argv) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(filename == NULL))
    return EINVAL;

  // the Clang API wants this as an int
  if (ERROR(argc > INT_MAX))
    return EINVAL;

  if (ERROR(argc > 0 && argv == NULL))
    return EINVAL;

  int rc = 0;

  // initialise Clang
  CXIndex index;
  CXTranslationUnit tu = NULL;
  if ((rc = init(&index, &tu, filename, argc, argv)))
    goto done;

  // get a top level cursor
  CXCursor root = clang_getTranslationUnitCursor(tu);

  // state for the traversal
  state_t state = {.db = db};

  // traverse from the root node
  (void)clang_visitChildren(root, visit, &state);
  rc = state.rc;

done:
  deinit(tu, index);

  return rc;
}
