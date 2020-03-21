#include <cstddef>
#include <clang-c/Index.h>
#include <clink/Error.h>
#include <clink/parse_c.h>
#include <errno.h>
#include <functional>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace clink {

// RAII wrapper for CXIndex
namespace { class ClangIndex {

 private:
  CXIndex index;

 public:
  ClangIndex() {
    static const int excludePCH = 0;
    static const int displayDiagnostics = 0;
    index = clang_createIndex(excludePCH, displayDiagnostics);
  }

  // implicitly coerce to a CXIndex
  operator CXIndex() const {
    return index;
  }

  ~ClangIndex() {
    clang_disposeIndex(index);
  }
}; }

// RAII wrapper for CXTranslationUnit
namespace { class ClangTranslationUnit {

 private:
  CXTranslationUnit tu;

 public:
  ClangTranslationUnit(ClangIndex &index, const std::string &filename,
      const std::vector<std::string> &clang_args) {

    // convert argument list to C strings
    std::vector<const char*> args{clang_args.size()};
    for (size_t i = 0; i < clang_args.size(); ++i)
      args[i] = clang_args[i].c_str();

    // parse the file
    CXErrorCode err = clang_parseTranslationUnit2(index, filename.c_str(),
      args.data(), args.size(), nullptr, 0,
      CXTranslationUnit_DetailedPreprocessingRecord|CXTranslationUnit_KeepGoing,
      &tu);

    if (err != CXError_Success)
      throw Error("failed to parse " + filename, static_cast<int>(err));
  }

  CXCursor get_cursor() const {
    return clang_getTranslationUnitCursor(tu);
  }

  ~ClangTranslationUnit() {
    clang_disposeTranslationUnit(tu);
  }
}; }

// RAII wrapper around CXString
namespace { class ClangString {

 private:
  CXString s;

 public:
  explicit ClangString(CXString s_): s(s_) { }

  std::string get_string() const {
    return clang_getCString(s);
  }

  // implicitly coerce to a std::string
  operator std::string() const {
    return get_string();
  }

  ~ClangString() {
    clang_disposeString(s);
  }
}; }

namespace { struct VisitState {
  std::function<int(const Symbol&)> callback;
  std::string parent;
  int error;
}; }

static CXChildVisitResult visit(CXCursor cursor, CXCursor, CXClientData data) {

  // retrieve the type of this symbol
  CXCursorKind kind = clang_getCursorKind(cursor);
  Symbol::Category category;
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
      category = Symbol::DEFINITION;
      break;
    case CXCursor_CallExpr:
    case CXCursor_MacroExpansion:
      category = Symbol::FUNCTION_CALL;
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
      category = Symbol::REFERENCE;
      break;
    case CXCursor_InclusionDirective:
      category = Symbol::INCLUDE;
      break;

    default: // something irrelevant
      return CXChildVisit_Recurse;
  }

  // retrieve the name of this thing
  ClangString text(clang_getCursorSpelling(cursor));

  // skip entities with no name
  if (text.get_string() == "")
    return CXChildVisit_Recurse;

  VisitState *st = static_cast<VisitState*>(data);

  // retrieve its location
  {
    CXSourceLocation loc = clang_getCursorLocation(cursor);
    unsigned lineno, colno;
    CXFile file = nullptr;
    clang_getSpellingLocation(loc, &file, &lineno, &colno, nullptr);

    // skip if we do not have a meaningful location for this symbol
    if (file == nullptr)
      return CXChildVisit_Recurse;

    ClangString filename(clang_getFileName(file));

    // tell the caller about this symbol
    Symbol symbol{ category, text, filename, lineno, colno, st->parent };
    st->error = st->callback(symbol);
    if (st->error != 0)
      return CXChildVisit_Break;
  }

  if (category == Symbol::DEFINITION) {
    // this is something that can serve as the parent to other things

    // duplicate the state, but with this symbol as the parent
    VisitState container = *st;
    container.parent = text;

    // visit child nodes of this definition
    bool stop = clang_visitChildren(cursor, visit, &container) != 0;

    // propagate any error that was triggered
    st->error = container.error;

    return stop ? CXChildVisit_Break : CXChildVisit_Continue;
  }

  return CXChildVisit_Recurse;
}

int parse_c(const std::string &filename,
    const std::vector<std::string> &clang_args,
    std::function<int(const Symbol&)> const &callback) {

  // check the file exists
  struct stat ignored;
  if (stat(filename.c_str(), &ignored) != 0)
    return errno;

  // check we can open it for reading
  if (access(filename.c_str(), R_OK) != 0)
    return errno;

  // create a Clang index
  ClangIndex index;

  // parse the file
  ClangTranslationUnit tu{index, filename, clang_args};

  // get a top level cursor
  CXCursor cursor = tu.get_cursor();

  // traverse the file
  VisitState st{ callback, "", 0 };
  (void)clang_visitChildren(cursor, visit, &st);

  // retrieve any error the traversal yielded
  return st.error;
}

}
