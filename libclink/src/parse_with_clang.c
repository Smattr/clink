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

  /// macro expansions we have seen but postponed defining
  clink_symbol_t *macro_expansions;
  size_t macro_expansions_length;

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
    if (ok_parent) {
      parent = ctext;
      DEBUG("setting parent %s for recursion", parent);
    }
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
    DEBUG("recursing past irrelevant cursor %d", (int)kind);
    return CXChildVisit_Recurse;
  }

  CXString filename = {0};

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

    filename = clang_getFileName(file);
    const char *fname = clang_getCString(filename);

    // Macro expansions are typically discovered in the first phase,
    // preprocessing when we have no known parent. So if we are in that
    // situation, save the information for this symbol and we will recover it
    // later when we come across its parent.
    state_t *st = state;
    if (kind == CXCursor_MacroExpansion && st->current_parent == NULL) {

      // construct a partially populated symbol
      clink_symbol_t symbol = {.category = category,
                               .name = strdup(name),
                               .lineno = lineno,
                               .colno = colno};
      if (ERROR(symbol.name == NULL)) {
        rc = ENOMEM;
        goto done;
      }

      // expand our collection of accrued macro expansions
      size_t len = st->macro_expansions_length + 1;
      clink_symbol_t *macros =
          realloc(st->macro_expansions, len * sizeof(macros[0]));
      if (ERROR(macros == NULL)) {
        clink_symbol_clear(&symbol);
        rc = ENOMEM;
        goto done;
      }

      macros[len - 1] = symbol;
      st->macro_expansions = macros;
      st->macro_expansions_length = len;

    } else {
      // add this symbol to the database
      rc = add_symbol(state, category, name, fname, lineno, colno);
      if (ERROR(rc != 0))
        goto done;

      // see if we can parent any prior macro expansions
      if (is_parent(cursor)) {

        // what is the start and end of this function etc?
        CXSourceRange range = clang_getCursorExtent(cursor);
        CXSourceLocation start = clang_getRangeStart(range);
        CXSourceLocation end = clang_getRangeEnd(range);

        // extract start into usable numbers
        CXFile start_file = NULL;
        unsigned start_lineno, start_colno;
        clang_getSpellingLocation(start, &start_file, &start_lineno,
                                  &start_colno, NULL);

        // extract end into usable numbers
        CXFile end_file = NULL;
        unsigned end_lineno, end_colno;
        clang_getSpellingLocation(end, &end_file, &end_lineno, &end_colno,
                                  NULL);

        // see which macros we can re-parent
        for (size_t i = 0; i < st->macro_expansions_length; ++i) {
          clink_symbol_t symbol = st->macro_expansions[i];
          if (symbol.lineno < start_lineno)
            continue;
          if (symbol.lineno == start_lineno && symbol.colno < start_colno)
            continue;
          if (symbol.lineno > end_lineno)
            continue;
          if (symbol.lineno == end_lineno && symbol.colno > end_colno)
            continue;

          symbol.parent = (char *)name;
          symbol.path = (char *)fname;
          rc = clink_db_add_symbol(st->db, &symbol);
          if (ERROR(rc != 0))
            goto done;

          // remove this pending macro expansion
          clink_symbol_clear(&st->macro_expansions[i]);
          for (size_t j = i; j + 1 < st->macro_expansions_length; ++j)
            st->macro_expansions[j] = st->macro_expansions[j + 1];
          --st->macro_expansions_length;

          --i;
        }
      }
    }
  }

done:
  DEBUG("processed symbol %s, rc = %d", name == NULL ? "<unnamed>" : name, rc);
  if (filename.data != NULL)
    clang_disposeString(filename);
  clang_disposeString(text);

  // abort visitation if we have seen an error
  if (rc)
    return CXChildVisit_Break;

  // recurse into the children
  rc = visit_children(state, cursor);
  if (rc) {
    DEBUG("visiting children failed, rc = %d", rc);
    return CXChildVisit_Break;
  }

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
  if (rc != 0)
    goto done;

  // process any unparented macro expansions
  for (size_t i = 0; i < state.macro_expansions_length; ++i) {
    clink_symbol_t s = state.macro_expansions[i];
    s.path = (char *)filename;
    rc = clink_db_add_symbol(db, &s);
    if (ERROR(rc != 0))
      goto done;
  }

done:
  for (size_t i = 0; i < state.macro_expansions_length; ++i)
    clink_symbol_clear(&state.macro_expansions[i]);
  free(state.macro_expansions);

  deinit(tu, index);

  return rc;
}
