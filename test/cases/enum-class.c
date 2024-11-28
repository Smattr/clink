/// does `enum class` trip up Cscope-based parsing?
///
/// Cscope treats `class` as a leader and expects a name following it. But
/// `class` is not a keyword in C, as opposed to C++. Because Cscope is agnostic
/// as to whether it is parsing C or C++, it emits two global definitions:
///   1. `class`, corresponding to the C semantics
///   2. An anonymous one (named ``)
/// It is unclear to me what the purpose of the second one is, because nothing
/// can reference this, but nevertheless it is written to Cscope’s database.

enum class { };

// RUN: clink --build-only --database={%t} --debug --parse-c=cscope {%s} >/dev/null
