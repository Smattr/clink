#pragma once

#include "AsmParser.h"
#include "CXXParser.h"

/* Management of C/C++ and assembly parsers that we'll lazily construct. It's
 * possible we'll not need one or both of them, in which case we don't have to
 * pay the construction overhead.
 *
 * Note that the implicit expectation is that these resources are thread-local.
 * The class assumes it does not have to do any mutual exclusion or
 * synchronisation.
 */
class Resources {

 public:
  Resources() noexcept {}

  // It doesn't make sense to copy one of these.
  Resources(const Resources &) = delete;

  Resources(Resources &&other) noexcept {
    CXXParser *cxx_parser = other.cxx_parser;
    AsmParser *asm_parser = other.asm_parser;
    other.cxx_parser = nullptr;
    other.asm_parser = nullptr;
    this->cxx_parser = cxx_parser;
    this->asm_parser = asm_parser;
  }

  ~Resources() {
    delete cxx_parser;
    delete asm_parser;
  }

  // As above, it doesn't make sense to copy one of these.
  Resources &operator=(const Resources &) = delete;

  Resources &operator=(Resources &&other) noexcept {
    CXXParser *cxx_parser = other.cxx_parser;
    AsmParser *asm_parser = other.asm_parser;
    other.cxx_parser = nullptr;
    other.asm_parser = nullptr;
    delete this->cxx_parser;
    delete this->asm_parser;
    this->cxx_parser = cxx_parser;
    this->asm_parser = asm_parser;
    return *this;
  }

  CXXParser *get_cxx_parser() {
    if (cxx_parser == nullptr)
      cxx_parser = new CXXParser;
    return cxx_parser;
  }

  AsmParser *get_asm_parser() {
    if (asm_parser == nullptr)
      asm_parser = new AsmParser;
    return asm_parser;
  }

 private:
  CXXParser *cxx_parser = nullptr;
  AsmParser *asm_parser = nullptr;

};
