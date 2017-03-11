#pragma once

#include "FileReader.h"
#include <string>
#include <unordered_map>

/* Shallow wrapper around a map of FileReaders.
 *
 * This is handy to avoid Database etc having to concern themselves with
 * tracking FileReaders and cleaning them up.
 */
class FileSystem {

 public:
  virtual ~FileSystem();

  /* Get the line (both unhighlighted and highlighted content) from the given
   * file. Note that the caller does not need to do anything specific prior to
   * this to warn FileSystem they are going to need lines from this file.
   */
  const Line &get_line(const std::string &filename, unsigned lineno);

 private:
  std::unordered_map<std::string, FileReader*> fr;

};
