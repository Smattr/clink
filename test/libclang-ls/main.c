/// \file
/// \brief parse a file using libclang
///
/// This is a utility for asking “how does libclang see this file?” It is
/// possible to approximate this using options to `clang` like `-emit-ast` but
/// you are never quite sure you are exploring exactly what a programmatic
/// invocation of libclang would encounter. Using this vehicle, you can
/// experiment with different Clang flags and see what is identified.

#include "../../common/compiler.h"
#include <assert.h>
#include <clang-c/Index.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static CXTranslationUnit tu;

static enum CXChildVisitResult visit(CXCursor cursor, CXCursor parent,
                                     CXClientData state) {

  // TODO: indicate the parent of cursors
  (void)parent;

  // we do not need any state
  (void)state;

  CXString filestr = {0};
  CXString kindstr = {0};
  CXString textstr = {0};

  // get the location of the cursor
  CXSourceLocation loc = clang_getCursorLocation(cursor);
  unsigned lineno, colno;
  CXFile file = NULL;
  clang_getSpellingLocation(loc, &file, &lineno, &colno, NULL);
  const char *filename = "<none>";
  if (file != NULL) {
    filestr = clang_getFileName(file);
    filename = clang_getCString(filestr);
  }

  // retrieve the type of this symbol
  enum CXCursorKind kind = clang_getCursorKind(cursor);
  kindstr = clang_getCursorKindSpelling(kind);
  assert(kindstr.data != NULL);
  const char *kindcstr = clang_getCString(kindstr);
  assert(kindcstr != NULL);

  // retrieve the text of this node
  textstr = clang_getCursorSpelling(cursor);
  assert(textstr.data != NULL);
  const char *textcstr = clang_getCString(textstr);

  printf("%s:%u:%u: %s with text «%s»\n", filename, lineno, colno, kindcstr,
         textcstr);

  if (textstr.data != NULL)
    clang_disposeString(textstr);
  if (kindstr.data != NULL)
    clang_disposeString(kindstr);
  if (filestr.data != NULL)
    clang_disposeString(filestr);

  // if this is a macro expansion, tokenise its contained content
  if (kind == CXCursor_MacroExpansion) {

    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = NULL;
    unsigned tokens_len = 0;
    clang_tokenize(tu, range, &tokens, &tokens_len);

    for (unsigned i = 0; i < tokens_len; ++i) {

      loc = clang_getTokenLocation(tu, tokens[i]);
      file = NULL;
      clang_getSpellingLocation(loc, &file, &lineno, &colno, NULL);
      filename = "<none>";
      if (file != NULL) {
        filestr = clang_getFileName(file);
        filename = clang_getCString(filestr);
      }
      printf(" %s:%u:%u: ", filename, lineno, colno);
      if (filestr.data != NULL)
        clang_disposeString(filestr);

      CXTokenKind k = clang_getTokenKind(tokens[i]);
      switch (k) {
      case CXToken_Punctuation:
        printf("CXToken_Punctuation");
        break;
      case CXToken_Keyword:
        printf("CXToken_Keyword");
        break;
      case CXToken_Identifier:
        printf("CXToken_Identifier");
        break;
      case CXToken_Literal:
        printf("CXToken_Literal");
        break;
      case CXToken_Comment:
        printf("CXToken_Comment");
        break;
      }

      CXString tokenstr = clang_getTokenSpelling(tu, tokens[i]);
      printf(" with text «%s»\n", clang_getCString(tokenstr));
      clang_disposeString(tokenstr);
    }

    clang_disposeTokens(tu, tokens, tokens_len);
  }

  return CXChildVisit_Recurse;
}
static const char *strcxerror(enum CXErrorCode err) {
  switch (err) {
  case CXError_Success:
    return "OK";
  case CXError_Failure:
    return "generic error";
  case CXError_Crashed:
    return "libclang crashed";
  case CXError_InvalidArguments:
    return "invalid arguments";
  case CXError_ASTReadError:
    return "AST deserialization error";
  }
  UNREACHABLE();
}

int main(int argc, char **argv) {

  int rc = EXIT_FAILURE;

  // create a Clang index
  static const int excludePCH = 0;
  static const int displayDiagnostics = 0;
  CXIndex index = clang_createIndex(excludePCH, displayDiagnostics);

  // ask Clang to parse what the user gave us
  unsigned options = 0;
  options |= CXTranslationUnit_DetailedPreprocessingRecord;
  options |= CXTranslationUnit_KeepGoing;
#if CINDEX_VERSION_MINOR >= 60
  options |= CXTranslationUnit_RetainExcludedConditionalBlocks;
#endif
  enum CXErrorCode err = clang_parseTranslationUnit2(
      index, NULL, (const char *const *)argv, argc, NULL, 0, options, &tu);
  if (err != CXError_Success) {
    fprintf(stderr, "%s:%d: failed: %s (%d)\n", __FILE__, __LINE__,
            strcxerror(err), (int)err);
    goto done;
  }

  // get a top level cursor
  CXCursor root = clang_getTranslationUnitCursor(tu);

  // walk the AST
  bool terminated = clang_visitChildren(root, visit, NULL) != 0;

  if (!terminated)
    rc = EXIT_SUCCESS;

done:
  if (tu != NULL)
    clang_disposeTranslationUnit(tu);
  clang_disposeIndex(index);

  return rc;
}
