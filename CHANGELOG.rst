Change log
==========

v2022.09.24
-----------
* The generic C, C++, MSVC DEF, and Python parsers now all recognise comment
  syntax (commits 8a80c768808f616c205b5fb39f1b4176bcf66dc4,
  0d7d0b2e7c46e898a90f4eda569f515c0997f8b3,
  e8b71beecb0fe2061cc5195c6ed7556696fe5a5a,
  a01295e13bac6d944ec0532be6e1bee578ef292c).
* The generic C and C++ parsers now recognise string and character literals
  (commits 8f5748f431edaa1bc26ebd1f349702af5ada020f,
  2b3aeff124c5084065416d39519936da4434fa78).
* The language struct passed to ``clink_parse_generic`` has a slightly different
  format (commit 71c2deb12ae6f4bc8be12fde1f696a136f9346f1).
* ``clink_parse_generic`` supports recognising comment syntax (commit
  8898445c3a5afa9e0bea72233240c9c92d367c20).
* The callback to ``clink_vim_read`` now receives a non-const string (commit
  244c295ade59d3d4a188f64cec9c70b7aa690b6e).
* White space in file content lines is left-trimmed before insertion into the
  database (commit dbc8aaa59e06de1f6fb2630c0abb16a35a33c456).

v2022.09.18
-----------
* Initial release.
