/// a macro definition should be considered the parent of its contained
/// references

#define FOO ref

#define BAR a_call(x)

#define BAZ /* a comment
               some more lines */ our_ref

// XFAIL: True

// RUN: clink --build-only --database {tmp} {__file__} >/dev/null

// RUN: echo 'select * from symbols where name = "ref";' | sqlite3 {tmp}
// CHECK: ref|{__file__}|2|4|13|FOO

// RUN: echo 'select * from symbols where name = "a_call";' | sqlite3 {tmp}
// CHECK: a_call|{__file__}|1|6|13|BAR

// RUN: echo 'select * from symbols where name = "our_ref";' | sqlite3 {tmp}
// CHECK: our_ref|{__file__}|2|9|35|BAZ
