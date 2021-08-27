#include <assert.h>
#include <clang-c/Index.h>
#include <clink/c.h>
#include <clink/iter.h>
#include <clink/symbol.h>
#include "../../common/compiler.h"
#include <errno.h>
#include "iter.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// a Clang node we are intending to traverse
typedef struct {
  char *parent; ///< named parent, grandparent, etc. of this node
  CXCursor cursor; ///< reference to the node itself
} node_t;

// state used by a C parsing iterator
typedef struct {

  /// Clang index used for parsing
  CXIndex index;

  /// Clang translation unit for the source file
  CXTranslationUnit tu;

  /// nodes to expand (descend into)
  node_t *pending;
  size_t pending_size;
  size_t pending_capacity;

  /// next symbols to yield
  clink_symbol_t *next;
  size_t next_size;
  size_t next_capacity;

  /// named parent of the current context during traversal
  const char *current_parent;

  /// status of our Clang traversal (0 OK, non-zero on error)
  int rc;

} iter_state_t;

static void deinit(CXTranslationUnit tu, CXIndex index) {
  if (tu != NULL)
    clang_disposeTranslationUnit(tu);

  clang_disposeIndex(index);
}

static void state_free(iter_state_t **s) {

  iter_state_t *ss = *s;

  if (ss == NULL)
    return;

  ss->current_parent = NULL;

  for (size_t i = 0; i < ss->next_size; ++i)
    clink_symbol_clear(&ss->next[i]);
  free(ss->next);
  ss->next = NULL;
  ss->next_size = ss->next_capacity = 0;

  for (size_t i = 0; i < ss->pending_size; ++i)
    free(ss->pending[i].parent);
  free(ss->pending);
  ss->pending = NULL;
  ss->pending_size = ss->pending_capacity = 0;

  deinit(ss->tu, ss->index);
  ss->tu = NULL;

  free(ss);
  *s = NULL;
}

static int push_cursor(void *state, CXCursor cursor) {

  iter_state_t *s = state;

  assert(s != NULL);

  // do we need to expand the pending collection?
  if (s->pending_size == s->pending_capacity) {
    size_t c = s->pending_capacity == 0 ? 1 : s->pending_capacity * 2;
    node_t *p = realloc(s->pending, c * sizeof(p[0]));
    if (UNLIKELY(p == NULL)) {
      s->rc = ENOMEM;
      return s->rc;
    }
    s->pending_capacity = c;
    s->pending = p;
  }

  assert(s->pending_size < s->pending_capacity);

  // determine if this cursor is a definition
  bool is_definition = false;
  switch (clang_getCursorKind(cursor)) {
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
      is_definition = true;
      break;
    default:
      // leave as-is
      break;
  }

  char *parent = NULL;

  // if this node is a definition, it can serve as a semantic parent to children
  if (is_definition) {

    // extract its name
    CXString text = clang_getCursorSpelling(cursor);
    const char *ctext = clang_getCString(text);

    // is this a valid name for a parent?
    bool ok_parent = ctext != NULL && strcmp(ctext, "") != 0;

    // duplicate it
    if (ok_parent)
      parent = strdup(ctext);

    // clean up
    clang_disposeString(text);

    if (UNLIKELY(ok_parent && parent == NULL)) {
      s->rc = ENOMEM;
      return s->rc;
    }

  // otherwise, default to the parent of the current context
  } else if (s->current_parent != NULL) {

    parent = strdup(s->current_parent);
    if (UNLIKELY(parent == NULL)) {
      s->rc = ENOMEM;
      return s->rc;
    }
  }

  // now we can enqueue it
  size_t i = s->pending_size;
  s->pending[i].parent = parent;
  s->pending[i].cursor = cursor;
  ++s->pending_size;

  return s->rc;
}

static node_t pop_cursor(iter_state_t *s) {

  assert(s != NULL);
  assert(s->pending_size > 0);

  // extract the head of the queue
  node_t n = s->pending[0];

  // shuffle the queue forwards, overwriting this with the new head
  memmove(s->pending, &s->pending[1],
    (s->pending_size - 1) * sizeof(s->pending[0]));

  // update the queue size
  --s->pending_size;

  return n;
}

static int push_symbol(void *state, clink_category_t category, const char *name,
    const char *path, unsigned lineno, unsigned colno) {

  iter_state_t *s = state;

  assert(s != NULL);

  // do we need to expand the next collection?
  if (s->next_size == s->next_capacity) {
    size_t c = s->next_capacity == 0 ? 1 : s->next_capacity * 2;
    clink_symbol_t *n = realloc(s->next, c * sizeof(n[0]));
    if (UNLIKELY(n == NULL)) {
      s->rc = ENOMEM;
      return s->rc;
    }
    s->next_capacity = c;
    s->next = n;
  }

  assert(s->next_size < s->next_capacity);

  clink_symbol_t sym = { 0 };

  sym.category = category;

  assert(name != NULL);
  sym.name = strdup(name);
  if (UNLIKELY(sym.name == NULL)) {
    s->rc = ENOMEM;
    goto done;
  }

  if (path != NULL) {
    sym.path = strdup(path);
    if (UNLIKELY(sym.path == NULL)) {
      s->rc = ENOMEM;
      goto done;
    }
  }

  sym.lineno = lineno;
  sym.colno = colno;

  if (s->current_parent != NULL) {
    sym.parent = strdup(s->current_parent);
    if (UNLIKELY(sym.parent == NULL)) {
      s->rc = ENOMEM;
      goto done;
    }
  }

  // symbols discovered during parsing are yielded without a context, so we
  // leave the context member NULL

  s->next[s->next_size] = sym;
  ++s->next_size;

done:
  if (s->rc)
    clink_symbol_clear(&sym);

  return s->rc;
}

static void pop_symbol(iter_state_t *s) {

  assert(s != NULL);

  // is the queue empty?
  if (s->next_size == 0)
    return;

  // free strings allocated within the queue head
  clink_symbol_clear(&s->next[0]);

  // shuffle the queue forward, overwriting this with the new head
  memmove(s->next, &s->next[1], (s->next_size - 1) * sizeof(s->next[0]));

  // update the size of the queue
  --s->next_size;
}

/** a method for yielding a discovered symbol in `visit`
 *
 * \param state State specific to this action
 * \param category Category of the discovered symbol
 * \param name Name of the symbol
 * \param fname Filename in which the symbol was found
 * \param lineno Line number within `fname` at which the symbol was found
 * \param colno Column number within `lineno` at which the symbol was found
 * \returns 0 on success or an errno on failure
 */
typedef int (*yielder)(void *state, clink_category_t category, const char *name,
                       const char *fname, unsigned lineno, unsigned colno);

/** a method for enqueuing a Clang cursor to expand in `visit`
 *
 * \param state State specific to this action
 * \param cursor Cursor to enqueue
 * \returns 0 on success or an errno on failure
 */
typedef int (*enqueuer)(void *state, CXCursor cursor);

static enum CXChildVisitResult visit(CXCursor cursor, void *state,
                                     yielder yield, enqueuer enqueue) {

  int rc = 0;

  // retrieve the type of this symbol
  enum CXCursorKind kind = clang_getCursorKind(cursor);
  clink_category_t category = CLINK_INCLUDE;
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
      return CXChildVisit_Continue;
  }

  // retrieve the name of this thing
  CXString text = clang_getCursorSpelling(cursor);
  const char *name = clang_getCString(text);

  // skip entities with no name
  if (name == NULL || strcmp(name, "") == 0)
    goto done;

  // retrieve its location
  {
    CXSourceLocation loc = clang_getCursorLocation(cursor);
    unsigned lineno, colno;
    CXFile file = NULL;
    clang_getSpellingLocation(loc, &file, &lineno, &colno, NULL);

    // skip if we do not have a meaningful location for this symbol
    if (file == NULL)
      goto done;

    CXString filename = clang_getFileName(file);
    const char *fname = clang_getCString(filename);

    // enqueue this symbol for future yielding
    rc = yield(state, category, name, fname, lineno, colno);

    clang_disposeString(filename);
  }

done:
  clang_disposeString(text);

  // abort visitation if we have seen an error
  if (rc)
    return CXChildVisit_Break;

  // enqueue this as a future cursor to expand
  rc = enqueue(state, cursor);
  if (rc)
    return CXChildVisit_Break;

  return CXChildVisit_Continue;
}

static enum CXChildVisitResult visit_iter(CXCursor cursor, CXCursor parent,
    CXClientData data) {

  // we do not need the parent cursor
  (void)parent;

  return visit(cursor, data, push_symbol, push_cursor);
}

/// expand a pending cursor and add more entries to the next queue
static int expand(iter_state_t *s) {

  assert(s != NULL);
  assert(s->pending_size > 0);

  // take the head of the pending cursors
  node_t n = pop_cursor(s);

  // update the current parent to mark that we are descending beneath this
  // cursor
  s->current_parent = n.parent;

  // expand this node, discovering its children
  (void)clang_visitChildren(n.cursor, visit_iter, s);

  // clear the current parent and discard this node
  s->current_parent = NULL;
  free(n.parent);

  return s->rc;
}

static int next(clink_iter_t *it, const clink_symbol_t **yielded) {

  if (UNLIKELY(it == NULL))
    return EINVAL;

  if (UNLIKELY(yielded == NULL))
    return EINVAL;

  iter_state_t *s = it->state;

  if (UNLIKELY(s == NULL))
    return EINVAL;

  int rc = 0;

  // discard the (last yielded) head of the next queue, if it exists
  pop_symbol(s);

  // refill the queue until we have an available symbol
  while (s->next_size == 0 && s->pending_size > 0) {
    if ((rc = expand(s)))
      return rc;
  }

  // have we exhausted this iterator?
  if (s->next_size == 0)
    return ENOMSG;

  // otherwise we have something to return
  *yielded = &s->next[0];

  return rc;
}

static void my_free(clink_iter_t *it) {

  if (it == NULL)
    return;

  if (it->state == NULL)
    return;

  state_free((iter_state_t**)&it->state);
}

// translate a libclang error into the closest errno equivalent
static int clang_err_to_errno(int err) {
  switch (err) {
    case CXError_Failure: return ENOTRECOVERABLE;
    case CXError_Crashed: return EIO;
    case CXError_InvalidArguments: return EINVAL;
    case CXError_ASTReadError: return EPROTO;
    default: return 0;
  }
}

static int init(CXIndex *index, CXTranslationUnit *tu, const char *filename,
    size_t argc, const char **argv) {

  // create a Clang index
  static const int excludePCH = 0;
  static const int displayDiagnostics = 0;
  *index = clang_createIndex(excludePCH, displayDiagnostics);

  // parse the input file
  enum CXErrorCode err = clang_parseTranslationUnit2(*index, filename, argv,
    argc, NULL, 0,
    CXTranslationUnit_DetailedPreprocessingRecord|CXTranslationUnit_KeepGoing,
    tu);
  if (err != CXError_Success)
    return clang_err_to_errno(err);

  return 0;
}

int clink_parse_c(clink_iter_t **it, const char *filename, size_t argc,
    const char **argv) {

  if (UNLIKELY(it == NULL))
    return EINVAL;

  if (UNLIKELY(filename == NULL))
    return EINVAL;

  if (UNLIKELY(argc > 0 && argv == NULL))
    return EINVAL;

  // check the file is readable
  if (access(filename, R_OK) < 0)
    return errno;

  // allocate iterator state
  iter_state_t *s = calloc(1, sizeof(*s));
  if (UNLIKELY(s == NULL))
    return ENOMEM;

  clink_iter_t *i = NULL;
  int rc = 0;

  if ((rc = init(&s->index, &s->tu, filename, argc, argv)))
    goto done;

  // get a top level cursor
  CXCursor root = clang_getTranslationUnitCursor(s->tu);

  // setup the initial iterator queue
  if (UNLIKELY((rc = push_cursor(s, root))))
    goto done;

  // create an iterator
  i = calloc(1, sizeof(*i));
  if (UNLIKELY(i == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // configure it to iterate over symbols of this translation unit
  i->next_symbol = next;
  i->state = s;
  s = NULL;
  i->free = my_free;

done:
  if (rc) {
    clink_iter_free(&i);
    state_free(&s);
  } else {
    *it = i;
  }

  return rc;
}
