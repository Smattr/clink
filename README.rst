Clink — a modern re-implementation of Cscope
============================================

When working in a large, complex C/C++ code base, an invaluable navigation tool
is Cscope_. However, Cscope is showing its age and has some issues that prevent
it from being the perfect assistant. Clink aims to be bring the Cscope
experience into the twenty first century.

What does that mean?
--------------------

* **Full C/C++ semantic support** – Clink uses libclang, which means it
  understands your code (including macros) as well as your compiler does.
* **Multicore support** – Parsing multiple, independent files trivially
  parallelises, so why be limited by one core?
* **Assembly, MSVC DEFs, Python support** – Your code base is more than just C
  and C++. Clink parses some other formats fuzzily and can maintain a call graph
  across language boundaries.
* **Syntax highlighting** – You’re probably used to looking at code in Vim with
  syntax highlighting, so Clink can ask Vim to highlight the snippets it shows
  you.
* **UTF-8** – Ever wanted to search ``#include`` of a header whose path
  contains non-ASCII characters? Now you can.
* **Exact jumps** – Clink opens Vim not only at the right line, but at the right
  column for the entry you’ve asked for.
* **Regex** - Can’t remember the exact name of a symbol? Want to search for
  multiple similar symbols at once? Clink supports extended regex in all search
  fields.
* **Fewer features** – Cscope’s options to find files and regex text are now
  better served by any__ number__ of__ other__ tools__ and are not included in
  Clink.

Building Clink
--------------

.. code-block:: sh

  # download Clink
  git clone --recurse-submodules https://github.com/Smattr/clink
  cd clink

  # configure and compile
  cmake -B build
  cmake --build build

Notes for devs
--------------

* Vim integration is currently hard coded. I didn’t make this parametric or
  implement any abstraction for this because Vim is my unabashed weapon of
  choice. If you want support for another editor, please ask me and I’ll
  probably do it.
* Cscope’s “find assignments to this symbol” is not implemented. Honestly, I
  have never used this query. Have you? It actually sounds really useful, but I
  have never once thought of this until enumerating Cscope’s options while
  implementing Clink.
* Some open questions about Cscope that I haven’t yet explored:

  * Why do Cscope’s line-oriented and curses interface results differ? The
    example I have on hand is a file that #includes a file from a parent
    directory. If I had to speculate, I’d say this is actually a Cscope bug.
    The discrepancy seems to lead to a worse user experience in Cscope (file
    jumping in Vim that doesn’t work) though admittedly I’ve never actually
    noticed this until staring at Cscope results while implementing Clink.

Anything else you don’t understand, ask away. Questions are the only way I
learned enough to write this thing.

Legal
-----
Everything in this repository is in the public domain, under the terms of
the Unlicense. For the full text, see LICENSE_.

.. _Cscope: http://cscope.sourceforge.net/
__ http://blog.burntsushi.net/ripgrep/
__ http://geoff.greer.fm/ag/
__ http://beyondgrep.com/
__ https://en.wikipedia.org/wiki/Grep
__ https://en.wikipedia.org/wiki/Sed
.. _LICENSE: ./LICENSE
