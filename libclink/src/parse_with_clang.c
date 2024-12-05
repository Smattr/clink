#include "../../common/ctype.h"
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

static int add(state_t *state, clink_category_t category, const char *name,
               const char *path, unsigned lineno, unsigned colno,
               clink_location_t start, clink_location_t end) {

  assert(state != NULL);

  clink_symbol_t symbol = {.category = category,
                           .name = (char *)name,
                           .path = (char *)path,
                           .lineno = lineno,
                           .colno = colno,
                           .start = start,
                           .end = end,
                           .parent = (char *)state->current_parent};
  state->rc = clink_db_add_symbol(state->db, &symbol);

  return state->rc;
}

/** add all tokens within the given cursor as references
 *
 * This is useful for processing a region of the source file Clang has no
 * semantic understanding of. The prime example is a macro definition. The
 * contents of something like this may have some inferred meaning to a human
 * reader, but have no exact meaning to the compiler except as interpreted at a
 * macro expansion site.
 *
 * This function filters out only identifiers, as punctuation, comments, etc are
 * not relevant.
 *
 * \param db Database to insert into
 * \param path Originating ource file path
 * \param parent Name of semantic parent, can be `NULL`
 * \param cursor Cursor to tokenize
 * \return 0 on success or an errno on failure
 */
static int add_tokens(clink_db_t *db, const char *path, const char *parent,
                      CXCursor cursor) {

  assert(db != NULL);
  assert(path != NULL);

  CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);

  int rc = 0;

  // find all the tokens covered by this cursor
  CXToken *tokens = NULL;
  unsigned tokens_len = 0;
  {
    const CXSourceRange range = clang_getCursorExtent(cursor);
    clang_tokenize(tu, range, &tokens, &tokens_len);
  }

  for (unsigned i = 0; i < tokens_len; ++i) {

    // we expect the first token to be the parent itself, so skip it
    if (i == 0) {
#ifndef NDEBUG
      {
        // retrieve the name of the parent
        CXString name = clang_getCursorSpelling(cursor);
        const char *namecstr = clang_getCString(name);

        // get the text of this token
        CXString text = clang_getTokenSpelling(tu, tokens[i]);
        const char *textcstr = clang_getCString(text);

        assert(strcmp(namecstr, textcstr) == 0 &&
               "first token is not parent’s name");

        clang_disposeString(text);
        clang_disposeString(name);
      }
#endif
      continue;
    }

    // skip anything that is not an identifier
    CXTokenKind kind = clang_getTokenKind(tokens[i]);
    if (kind != CXToken_Identifier)
      continue;

    // get the position of this token
    clink_location_t start = {0};
    clink_location_t end = {0};
    {
      const CXSourceRange range = clang_getTokenExtent(tu, tokens[i]);
      {
        const CXSourceLocation range_start = clang_getRangeStart(range);
        unsigned lineno, colno, byte;
        clang_getSpellingLocation(range_start, NULL, &lineno, &colno, &byte);
        start =
            (clink_location_t){.lineno = lineno, .colno = colno, .byte = byte};
      }
      {
        const CXSourceLocation range_end = clang_getRangeEnd(range);
        unsigned lineno, colno, byte;
        clang_getSpellingLocation(range_end, NULL, &lineno, &colno, &byte);
        assert(colno > 1 && "libclang gave us a column number that appears "
                            "inclusive instead of exclusive");
        assert(byte > start.byte && "libclang gave us a byte offset that "
                                    "appears inclusive instead of exclusive");
        end = (clink_location_t){
            .lineno = lineno, .colno = colno - 1, .byte = byte - 1};
      }
    }

    // get the text of this token
    CXString text = clang_getTokenSpelling(tu, tokens[i]);
    const char *textcstr = clang_getCString(text);

    clink_symbol_t symbol = {.category = CLINK_REFERENCE,
                             .name = (char *)textcstr,
                             .path = (char *)path,
                             .lineno = start.lineno,
                             .colno = start.colno,
                             .start = start,
                             .end = end,
                             .parent = (char *)parent};
    rc = clink_db_add_symbol(db, &symbol);

    clang_disposeString(text);

    if (ERROR(rc != 0))
      goto done;
  }

done:
  clang_disposeTokens(tu, tokens, tokens_len);

  return rc;
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

/// if this looks like a call, extract the callee’s name
static char *get_callee(CXCursor cursor) {
  enum CXCursorKind kind = clang_getCursorKind(cursor);
  assert(kind == CXCursor_UnexposedExpr || kind == CXCursor_CallExpr);
  (void)kind;

  CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);

  char *rc = NULL;

  // find all the tokens covered by this cursor
  CXSourceRange range = clang_getCursorExtent(cursor);
  CXToken *tokens = NULL;
  unsigned tokens_len = 0;
  clang_tokenize(tu, range, &tokens, &tokens_len);

  // the call must at least contain «callee»«(»«)»
  if (tokens_len < 3)
    goto done;

  // the second token must be the opening paren
  {
    CXString paren = clang_getTokenSpelling(tu, tokens[1]);
    const char *paren_cstr = clang_getCString(paren);

    bool is_paren = strcmp(paren_cstr, "(") == 0;
    clang_disposeString(paren);
    if (!is_paren)
      goto done;
  }

  // the last token must be the closing paren
  {
    CXString paren = clang_getTokenSpelling(tu, tokens[tokens_len - 1]);
    const char *paren_cstr = clang_getCString(paren);

    bool is_paren = strcmp(paren_cstr, ")") == 0;
    clang_disposeString(paren);
    if (!is_paren)
      goto done;
  }

  // the first token must be a valid identifier
  {
    CXString callee = clang_getTokenSpelling(tu, tokens[0]);
    const char *callee_cstr = clang_getCString(callee);

    bool is_id = strcmp(callee_cstr, "") != 0;
    for (size_t i = 0; callee_cstr[i] != '\0'; ++i) {
      if (isalpha_(callee_cstr[i]))
        continue;
      if (callee_cstr[i] == '_')
        continue;
      if (i != 0 && isdigit_(callee_cstr[i]))
        continue;
      is_id = false;
      break;
    }

    if (is_id)
      rc = strdup(callee_cstr);

    clang_disposeString(callee);
    if (!is_id)
      goto done;
  }

done:
  clang_disposeTokens(tu, tokens, tokens_len);
  return rc;
}

// is this the name of an implementation of one of the __sync built-ins?
static bool is_sync_impl(const char *name) {
  if (name == NULL)
    return false;

  // check this looks like a __sync built-in
  const char *suffix = NULL;
#define MATCH(prefix)                                                          \
  do {                                                                         \
    if (suffix == NULL) {                                                      \
      if (strncmp(name, ("__sync_" prefix "_"),                                \
                  strlen("__sync_" prefix "_")) == 0) {                        \
        suffix = name + strlen("__sync_" prefix "_");                          \
      }                                                                        \
    }                                                                          \
  } while (0)
  MATCH("fetch_and_add");
  MATCH("fetch_and_sub");
  MATCH("fetch_and_or");
  MATCH("fetch_and_and");
  MATCH("fetch_and_xor");
  MATCH("fetch_and_nand");
  MATCH("add_and_fetch");
  MATCH("sub_and_fetch");
  MATCH("or_and_fetch");
  MATCH("and_and_fetch");
  MATCH("xor_and_fetch");
  MATCH("nand_and_fetch");
  MATCH("bool_compare_and_swap");
  MATCH("val_compare_and_swap");
  MATCH("lock_test_and_set");
  MATCH("lock_release");
#undef MATCH

  if (suffix == NULL)
    return false;

  // the remainder should be a bit width
  if (*suffix == '\0')
    return false;
  for (; *suffix != '\0'; ++suffix) {
    if (!isdigit_(*suffix))
      return false;
  }

  return true;
}

/// does libclang have `CXBinaryOperator…` and `CXUnaryOperator…`?
#define HAS_OPS                                                                \
  (CINDEX_VERSION_MAJOR > 0 ||                                                 \
   (CINDEX_VERSION_MAJOR == 0 && CINDEX_VERSION_MINOR >= 64))

#if HAS_OPS
/// if this looks like an assignment to a symbol, extract the symbol’s name
static char *get_assign_lhs(CXCursor cursor) {
  {
    const enum CXCursorKind kind = clang_getCursorKind(cursor);
    assert(kind == CXCursor_BinaryOperator);
    (void)kind;
  }

  CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);

  char *rc = NULL;

  // find all the tokens covered by this cursor
  CXSourceRange range = clang_getCursorExtent(cursor);
  CXToken *tokens = NULL;
  unsigned tokens_len = 0;
  clang_tokenize(tu, range, &tokens, &tokens_len);

  // ignore anything that is not «symbol»«assign op»…
  if (tokens_len < 2)
    goto done;
  {
    const enum CXBinaryOperatorKind kind =
        clang_getCursorBinaryOperatorKind(cursor);
    CXString expected = clang_getBinaryOperatorKindSpelling(kind);
    const char *const exp = clang_getCString(expected);
    CXString actual = clang_getTokenSpelling(tu, tokens[1]);
    const char *const act = clang_getCString(actual);

    const bool is_simple = strcmp(exp, act) == 0;
    clang_disposeString(actual);
    clang_disposeString(expected);
    if (!is_simple)
      goto done;
  }

  // the first token must be a valid identifier
  {
    CXString lhs = clang_getTokenSpelling(tu, tokens[0]);
    const char *const lhs_cstr = clang_getCString(lhs);

    bool is_id = strcmp(lhs_cstr, "") != 0;
    for (size_t i = 0; lhs_cstr[i] != '\0'; ++i) {
      if (isalpha_(lhs_cstr[i]))
        continue;
      if (lhs_cstr[i] == '_')
        continue;
      if (i != 0 && isdigit_(lhs_cstr[i]))
        continue;
      is_id = false;
      break;
    }

    if (is_id) {
      rc = strdup(lhs_cstr);
    } else {
      DEBUG("binary token \"%s\" is not a valid symbol; skipping", lhs_cstr);
    }

    clang_disposeString(lhs);
    if (!is_id)
      goto done;
  }

done:
  clang_disposeTokens(tu, tokens, tokens_len);
  return rc;
}

/// is this a binary operator that counts as assignment?
static bool is_assign(enum CXBinaryOperatorKind op) {
  switch (op) {
  case CXBinaryOperator_Assign:
  case CXBinaryOperator_MulAssign:
  case CXBinaryOperator_DivAssign:
  case CXBinaryOperator_RemAssign:
  case CXBinaryOperator_AddAssign:
  case CXBinaryOperator_SubAssign:
  case CXBinaryOperator_ShlAssign:
  case CXBinaryOperator_ShrAssign:
  case CXBinaryOperator_AndAssign:
  case CXBinaryOperator_XorAssign:
  case CXBinaryOperator_OrAssign:
    return true;
  default:
    break;
  }
  return false;
}

/// is this a unary operator that counts as assignment?
static bool is_assign_unary(enum CXUnaryOperatorKind op) {
  switch (op) {
  case CXUnaryOperator_PostInc:
  case CXUnaryOperator_PostDec:
  case CXUnaryOperator_PreInc:
  case CXUnaryOperator_PreDec:
    return true;
  default:
    break;
  }
  return false;
}

/// if this looks like a unary assignment to a symbol, extract the symbol’s name
static char *get_assign_unary(CXCursor cursor) {
  {
    const enum CXCursorKind kind = clang_getCursorKind(cursor);
    assert(kind == CXCursor_UnaryOperator);
    (void)kind;
  }

  CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);

  char *rc = NULL;

  // find all the tokens covered by this cursor
  CXSourceRange range = clang_getCursorExtent(cursor);
  CXToken *tokens = NULL;
  unsigned tokens_len = 0;
  clang_tokenize(tu, range, &tokens, &tokens_len);

  // ignore anything that is not «symbol»«assign op» or «assign op»«symbol»
  if (tokens_len != 2) {
    DEBUG("%u tokens in unary expression; skipping", tokens_len);
    goto done;
  }

  // figure out in which position to look for the symbol
  unsigned symbol_position;
  {
    const enum CXUnaryOperatorKind kind =
        clang_getCursorUnaryOperatorKind(cursor);
    assert(is_assign_unary(kind));
    symbol_position =
        (kind == CXUnaryOperator_PreInc || kind == CXUnaryOperator_PreDec) ? 1
                                                                           : 0;
  }

  // the token must be a valid identifier
  {
    CXString symbol = clang_getTokenSpelling(tu, tokens[symbol_position]);
    const char *const sym = clang_getCString(symbol);

    bool is_id = strcmp(sym, "") != 0;
    for (size_t i = 0; sym[i] != '\0'; ++i) {
      if (isalpha_(sym[i]))
        continue;
      if (sym[i] == '_')
        continue;
      if (i != 0 && isdigit_(sym[i]))
        continue;
      is_id = false;
      break;
    }

    if (is_id) {
      rc = strdup(sym);
    } else {
      DEBUG("unary token \"%s\" is not a valid symbol; skipping", sym);
    }

    clang_disposeString(symbol);
    if (!is_id)
      goto done;
  }

done:
  clang_disposeTokens(tu, tokens, tokens_len);
  return rc;
}
#endif

static enum CXChildVisitResult visit(CXCursor cursor, CXCursor parent,
                                     CXClientData state) {

  // we do not need the parent cursor
  (void)parent;

  int rc = 0;

  // ignore anything not from the main file
  CXSourceLocation loc = clang_getCursorLocation(cursor);
  if (!clang_Location_isFromMainFile(loc))
    return CXChildVisit_Recurse;

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
#if HAS_OPS
  case CXCursor_BinaryOperator: {
    const enum CXBinaryOperatorKind op =
        clang_getCursorBinaryOperatorKind(cursor);
    if (is_assign(op)) {
      category = CLINK_ASSIGNMENT;
      break;
    }
    DEBUG("recursing past irrelevant cursor %d", (int)kind);
    return CXChildVisit_Recurse;
  }
  case CXCursor_UnaryOperator: {
    const enum CXUnaryOperatorKind op =
        clang_getCursorUnaryOperatorKind(cursor);
    if (is_assign_unary(op)) {
      category = CLINK_ASSIGNMENT;
      break;
    }
    DEBUG("recursing past irrelevant cursor %d", (int)kind);
    return CXChildVisit_Recurse;
  }
#endif

  default: // something irrelevant
    DEBUG("recursing past irrelevant cursor %d", (int)kind);
    return CXChildVisit_Recurse;
  }

  CXString filename = {0};

  // retrieve the name of this thing
  CXString text = clang_getCursorSpelling(cursor);
  const char *name = clang_getCString(text);
  char *extra_name = NULL;

  if (name != NULL) {
    // XXX: calls to the __atomic built-ins somehow only appear as unexposed
    // expressions
    bool maybe_atomic = kind == CXCursor_UnexposedExpr && strcmp(name, "") == 0;
    // XXX: similarly, calls to the __sync built-ins somehow appear as calls to
    // their byte-width implementations
    bool sync_impl = kind == CXCursor_CallExpr && is_sync_impl(name);
    if (maybe_atomic || sync_impl) {
      extra_name = get_callee(cursor);
      if (extra_name != NULL && strncmp(extra_name, "__", 2) == 0) {
        category = CLINK_FUNCTION_CALL;
        name = extra_name;
      }
    }
  }

#if HAS_OPS
  // for a pre-/post-increment/decrement, try to extract the name of the symbol
  // being incremented/decremented
  if (kind == CXCursor_UnaryOperator) {
    extra_name = get_assign_unary(cursor);
    if (extra_name != NULL) {
      name = extra_name;
      DEBUG("unravelled unary operator to assignment to symbol \"%s\"", name);
    } else {
      DEBUG("failed to unravel unary assignment operator");
    }
  }

  // for assignments, try to extract the name of the symbol being assigned to
  if (kind == CXCursor_BinaryOperator) {
    extra_name = get_assign_lhs(cursor);
    if (extra_name != NULL) {
      name = extra_name;
      DEBUG("unravelled binary operator to assignment to symbol \"%s\"", name);
    } else {
      DEBUG("failed to unravel binary assignment operator");
    }
  }
#endif

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

    // retrieve the range of the covering semantic entity
    clink_location_t start = {0};
    clink_location_t end = {0};
    {
      const CXSourceRange range = clang_getCursorExtent(cursor);
      {
        const CXSourceLocation range_start = clang_getRangeStart(range);
        unsigned l, c, b;
        clang_getSpellingLocation(range_start, NULL, &l, &c, &b);
        start = (clink_location_t){.lineno = l, .colno = c, .byte = b};
      }
      {
        const CXSourceLocation range_end = clang_getRangeEnd(range);
        unsigned l, c, b;
        clang_getSpellingLocation(range_end, NULL, &l, &c, &b);
        assert(c > 1 && "libclang gave us a column number that appears "
                        "inclusive instead of exclusive");
        assert(b > start.byte && "libclang gave us a byte offset that appears "
                                 "inclusive instead of exclusive");
        end = (clink_location_t){.lineno = l, .colno = c - 1, .byte = b - 1};
      }
    }

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
                               .colno = colno,
                               .start = start,
                               .end = end};
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
      rc = add(state, category, name, fname, lineno, colno, start, end);
      if (ERROR(rc != 0))
        goto done;

      // see if we can parent any prior macro expansions
      if (is_parent(cursor)) {

        // see which macros we can re-parent
        for (size_t i = 0; i < st->macro_expansions_length; ++i) {
          clink_symbol_t symbol = st->macro_expansions[i];
          if (symbol.lineno < start.lineno)
            continue;
          if (symbol.lineno == start.lineno && symbol.colno < start.colno)
            continue;
          if (symbol.lineno > end.lineno)
            continue;
          if (symbol.lineno == end.lineno && symbol.colno > end.colno)
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

      // If this is a macro definition, it will have been discovered in the
      // initial preprocessing phase. We get no semantic information about its
      // “children” because they are just lexical tokens. So tokenize it and
      // treat each seen identifier as a reference.
      if (kind == CXCursor_MacroDefinition) {
        rc = add_tokens(st->db, fname, name, cursor);
        if (ERROR(rc != 0))
          goto done;
      }
    }
  }

done:
  DEBUG("processed symbol %s, rc = %d", name == NULL ? "<unnamed>" : name, rc);
  if (filename.data != NULL)
    clang_disposeString(filename);
  free(extra_name);
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

  if (ERROR(filename[0] != '/'))
    return EINVAL;

  // the Clang API wants this as an int
  if (ERROR(argc > INT_MAX))
    return EINVAL;

  if (ERROR(argc > 0 && argv == NULL))
    return EINVAL;

  int rc = 0;

  // state for the traversal
  state_t state = {.db = db};

  // initialise Clang
  CXIndex index;
  CXTranslationUnit tu = NULL;
  if ((rc = init(&index, &tu, filename, argc, argv)))
    goto done;

  // get a top level cursor
  CXCursor root = clang_getTranslationUnitCursor(tu);

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
