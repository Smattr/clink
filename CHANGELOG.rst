Change log
==========

v2024.07.21
-----------
* Bug fix: record insertion failures during database construction are no longer
  ignored but instead surface as errors (commit
  23cec2bf6fd8f86c4afa92d2923567c417695019).
* Bug fix: single-quotes are used to delimit strings in SQL queries. This fixes
  a problem wherein stricter SQLite installations would reject Clink queries
  because technically double-quotes are not valid string delimiters (commit
  e25a260ae0b6dff7f7d27da0c9d0e527039053a9).
* Bug fix: close-on-exec is set on file descriptors that were not intended to be
  inherited by subprocesses (commits aefcb45070bc4b938ce4d7a06e9d8730362360d2,
  f5deb08121c7789cfabe79f4f103ef52f138ca77,
  6fbb005521b6389bcaeff4a9c5d5f1dc1b721517,
  6754b47cf95871ac5605b1b05abc8006fe899434,
  f07bc4ea3ba88a0d95ca89405310367a6c34147f,
  fc884782c8e5abacbe2408cfaf4b9a22b0a8f472,
  36fb96ce989a83230df67651418e714c637dfdf8,
  bb4754eda396c7c8e3ff552ea553a6b6285db2bf).
* Bug fix: null pointer dereferences during out-of-memory handling (commits
  29d5cf21586c5e1345fdbf56f5788ac765eda241,
  435e87a5cbcc225d9cec7ef6cc5308e8648c6a0e).
* Cscope’s ``-inamefile`` option is implemented. This is also accessible through
  a libclink function, ``clink_parse_namefile`` (commits
  b274ac9f21ca8ef0e152644c12ad6d52d0b3648d,
  b1c2ee34b9e8f3fdaae668e22141b4b3a01db8ae).
* Parsing of compile_commands.json has been improved (commits
  2a88bdcc53cbfd8a3f6e3dfffef51f04456f95cd,
  84ee5f9ce088c70cc0636ec88c5f970d4f881ab8,
  9a220563b5ad844b8132764d22f8c3ff0b26ddbb,
  b2d1848faafd716597e473a4894b9e2ec813d4e8,
  f02256c418fc298a2f3a6f836daac61c9281b06d,
  54ef6d38f5755f52cf6f75f3c6119222c111fa9a).
* Clink now opens files in the user’s preferred editor instead of assuming Vim.
  This also comes with a libclink function, ``clink_editor_open``, for opening
  the selected editor (commits 4d857dba320ead21d4e855cbb61cb1326a0a922a,
  29fc81116ca0d8f78443ad6916ceb40fda214c32,
  cac36c4ffc2e7005155c4f7322fc8da8fad4f448).
* A libclink function for inspecting the current editor,
  ``clink_is_editor_vim``, has been added (commit
  fc0ca8731023f89638958ef9fe8c5e1beff558d0).
* The list of keywords recognised when parsing C has been updated to support C23
  (commit 0a5aee3ff258304f31e37df4be0a63d1b560e3d7).
* Parsing ignores the active locale when determining identifier/digit characters
  (commit fc0d59f52be19c8af4b8918d0e5889a67d3e85f1).

v2023.11.13
-----------
* Bug fix: Symbol visibility for ``clink_parse_with_clang`` was corrected
  (commit cf6dd5943f9c144d04a8f8be0a953da61805093a).
* Pressing F5 in the UI rescans files/directories, updating the database for
  those that have changed (commit cb00f805c2dff8d81e97359ad82a4d2b072b786d).
* Man page typography has been improved (commit
  48e31a7a77c93687057d767c9b321f094b11d7e8).
* Use of the non-portable ``PATH_MAX`` constant has been removed (commits
  94fcf2003045a294255bef7949dc9da50d5c6f7b,
  5fe9c1236a45fbe92d5eb73361ccd4d2f6db748a).

v2023.06.04
-----------
* Paths stored in the database are now relative. This allows you to relocate a
  directory containing source code and a Clink database and have the database
  remain usable (commits 07770410c8af0eae8bf3b2e541d74eb19083454b,
  fb727720a0c3088b4b5e693d428d7c62ced18598,
  e4feb93935e6e80b26f7cc49122ff90e7b960f57,
  04d1e1af74b579da64dd692f28c62f543107fcdb,
  06ca43b6daa462514b6ed0423e1eefa0bdff3fb3
  b7c3568dcae38666bae166bc77f2e9b00f6c250b).
* Input paths to the API are now required to be absolute (commit
  04d1e1af74b579da64dd692f28c62f543107fcdb).
* Results shown in Vim when jumping between files now use relative paths where
  possible (commit 5ed239885a59f5009a16503e1ae8a99199c8a893).
* A graphical progress bar is displayed during database construction. Disabling
  animations (``--animation=off``) turns this off (commit
  0466adb14976a78b915fa9dfc1d0886ee72f015b).
* The database format has changed to normalise some tables (commit
  111e049fb047f9bc7ea270f694124cf85b805ffe.
* Syntax highlighting has been slightly accelerated (commit
  6e6e79327f48a1a8d9c4c9cdbfb76040628ebf65).

v2023.04.20
-----------
* Bug fix: some symbols that occur within parameters to macros that are
  themselves defined in an external header are now detected correctly when using
  Clang parsing (commit 44f37f40867f4c734d77927a10246850818f772b).
* Support for Lex/Flex files has been added (commit
  a9591cd02fddb5de7106208ce890a67808aa9f4c).
* Support for Yacc/Bison files has been added (commit
  820ce31d0701dadb0df42dd64c68f2d79026de09).
* Support for LLVM TableGen files has been added (commits
  8de1d9f6d71e39ea60e7df0c4ae553795ccad6ac,
  0f28dc557f0d15b34d1f60eecfb4e3de7c5ca67d,
  a073fe0b133ecdc0fdfd08ddbf2a51dbf64ec879).
* Search results that appear to be duplicated rows are now coalesced (commit
  b2c7fa71b2f13b016a9ee6d03f2f8715d7125df6).

v2023.04.16
-----------
* Bug fix: in Vim, when using the Cscope bridge functionality to jump to
  results, the results provided by Clink now always have their line content
  included (commit d66752359f3241b0bd9c7bf68dd6b77936b45a55).
* Bug fix: query text remains visible when returning from Vim (commit
  29ece0d347351f2e6709d15b9757ceb3ecb14aeb).
* Bug fix: Database pragmas are now enforced correctly. This could have
  previously resulted in performance degradation (commit
  51e7e6ca34898f39c8e0e2fbb7f7efa1fe220f8d).
* API break: ``clink_db_add_record`` gained an output parameter for record
  identifier. This can be used as an opaque reference to optimise later
  Cscope-based parsing through ``clink_parse_with_cscope`` which gained a new
  parameter (commits 5b3e991878c22bdc38bf0348e9f5dd126a4afd14,
  471f95fba82fcc952236026a90ce8bf43b64d8d1).
* Database files are now versioned. Attempting to open a database generated by a
  different incompatible version of Clink will be rejected. When upgrading
  through a change that alters the database version, existing databases will
  need to be regenerated (commits 2e1c518b74fa85aa8c74f18c825ea5a2cb610da1,
  12b35720f4e6f4b032d8e2645be20ce437139c18,
  5d387af62de20e5597f5133c6174e5108a100aab,
  73adbb1aef1b6637926b30f36463a386c74a3fd2).
* Database creation is up to 77% faster. Database files are up to 65% smaller
  (commits 66c5ea825b33e050ead9d95f692e0e5649f5af0d,
  5b3e991878c22bdc38bf0348e9f5dd126a4afd14,
  459144bcbffbd01064fce5bec2335324c4ff266e,
  cc0a7c12fc8af4356779f22906d149138279a4b2,
  c93b46bfacd92a7c3fbadd2c1d0a2c13988315fe,
  92f4b434071e05784e7eb42d1ef089f1449daf38,
  0daef051b0ac30160cca625fe54401e4778c5917,
  15068e861b9339da792e3745a8f8c51c446ee7b3,
  471f95fba82fcc952236026a90ce8bf43b64d8d1,
  319369a9c7c650bc66877c7d9710876bf74fccfe).

v2023.03.31
-----------
* Bug fix: syntax highlighted results in the UI are no longer affected by the
  highlighting of your last search term in Vim (commit
  642817e2b3fb3af2a0bfb02dfd07cea8e5bf9a7a).
* Database construction when using Cscope for parsing is ~16% faster (commits
  12e8e3113855a98cf9dd32bd2c6ca4688ca2f4fa,
  15d4bcc6b80b832c3b5192b7b6f015337e3705c2).

v2023.01.30
-----------
* Bug fix: pressing space in the UI view no longer advances too many entries
  (commit 3fe36b8f81ce4f78386ef4252bbef852ee26e9b6).
* Calls to the ``__atomic`` built-ins are now correctly detected when parsing in
  Clang mode (commit ff9dfb76daad6f43bb3dcbf8f612a75c1e9b05e9).
* Calls to the ``__sync`` built-ins are now correctly detected when parsing in
  Clang mode (commit 5a03a46542eaf1a4e54603c53d1caf370ac76f84).
* On-demand syntax highlighting that occurs in the UI now uses multiple threads
  (commit 45404b1844df0a13a7343b0b8c4aef2dac2fa67e).

v2023.01.01
-----------
* Bug fix: mouse pasting and middle-click pasting into the UI works once again
  (commit 17ea019394edf56baba8de784efd6dc8f0a28cd2).
* The ``\r`` and ``\n`` characters are no longer permitted in comment delimiters
  when describing a language to parse with the generic parser (commit
  a9f8a5974cb42d5f5cf968720fbf69b0e3059e0e).

v2022.12.27
-----------
* A new option, ``--script``, for automating Clink has been added. See the man
  page for details (commit 69b4a46ca6a3fb0431851a3b7d4bc7cfd7f9b4fb).
* libclink gained functions for using transactions on the underlying SQLite
  database (commit 91e296ca959d2e383ef89058891a8d3d05a8663a).
* Several performance improvements have made both indexing and searching faster
  (commits 7196725910c2bba3c1fe69a9bad58993b6a36dc5,
  b50bf2b1b85abb31843ae63432c796e92c4b3fed,
  ce07dd445992855872c679f44a1ce43910f53356,
  5de623d08b05ab61a5e39d83b5bbab87a94e443b).

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
