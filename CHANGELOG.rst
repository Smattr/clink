Change log
==========

v2022.12.04
-----------
* Bug fix: the ``--parse-cxx=clang`` command line option correctly sets the C++
  parsing mode instead of mistakenly setting the C parsing mode.
* ``--parse-c`` and ``--parse-cxx`` gained a new option that is also the new
  default, ``auto``. In this mode, ``clang`` is selected if a
  compile_commands.json is found or Cscope is not installed. Otherwise
  ``cscope`` is selected (commits a2a50c6f314fc9035694b7e0110dc8897c1d46c3,
  3fec779f6a9a18daa3a2661abc204704fe29ab8b,
  6d4275cca3c01040804dafeee79b2efb927e5ace).
* The file content lines stored in the database are now restricted to only those
  containing referenced symbols. The only user-visible effect for ``clink``
  users should be reduced on-disk database size and improved runtime (commit
  a14aeda4b6fb0f5422fc648efb144bad289d8bde).

v2022.10.29
-----------
* Bug fix: it is no longer possible to navigate off the bottom of the result
  list or select results on the next page (commit series merged in
  09cfbd140582d7d5b89163ce5d3adac6034affeb).
* Bug fix: Vim returning a non-zero exit status no longer causes Clink to exit
  or crash (commit series merged in b17e61fe61c02753c6876a3999d36c52a4ce2709).
* The selected row in the result list is now highlighted in blue. This behaviour
  is controllable with the ``--colour`` command line option (commit
  7d76f3a52a917b1f5dfbf78210dc6928338e7fcb).
* Paths in the result list are shown relative to the current directory instead
  of absolute (commit 83300fe55acb2cba5690a3c337a7dac3c8430178).
* Text animations can now be disabled through the ``--animation`` command line
  option. See ``--help`` for more information (commit series merged in
  85e52cd4ee1639f58e2ea8c446a2c109ff935f57).
* LLVM 15 is now supported in the build system (commit series merged in
  60fedb806aded6b10973ee618bc122e6b3712a69).

v2022.10.22
-----------
* ``clink`` can now locate a ``clink-repl`` adjacent to itself and pass this to
  Vim instead of assuming ``clink-repl`` can be found through ``$PATH``. This
  means the location you install Clink to no longer needs to be added to
  ``$PATH`` to avoid errors when ``clink`` opens Vim (commit series merged in
  afdfddca57885c363e518a4923d28f7024124a9e).
* Vim is checked for on startup and an error is shown, rather than trying to run
  Vim later and failing confusingly if it is not installed (commit
  7ac2d7c24ec933e67d29011ef122dcb94dddbb2c).
* Filenames beginning with characters like ``-`` no longer cause problems when
  opening Vim (commit 4375fa9c1fe02c861a7655f584e54854c7e4d393).

v2022.10.15
-----------
* The ``--parse-c`` and ``--parse-cxx`` options gained a new possible argument,
  ``cscope`` that uses Cscope to parse sources. This can be useful in a foreign
  project whose build flags you do not know and thus libclang struggles to parse
  accurately (commits merged in 874a2b894e91e227e9a94007f3ec08c42d289d71).
* Exit status from ``clink`` now follows sysexits.h guidelines more closely
  (commit 4c16e47b7a1c42f46615fbba67f0c1def4225a10).

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
