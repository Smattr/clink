#include "FileSystem.h"
#include <string>

using namespace std;

const Line &FileSystem::get_line(const string &filename, unsigned lineno) {
  FileReader *reader;

  auto it = fr.find(filename);

  if (it == fr.end()) {
    // We do not have this file open; open it now.
    reader = new FileReader(filename);
    fr[filename] = reader;

  } else {
    // We already had this file open.
    reader = it->second;
  }

  return reader->get_line(lineno);
}

FileSystem::~FileSystem() {
  for (auto it : fr)
    delete it.second;
}
