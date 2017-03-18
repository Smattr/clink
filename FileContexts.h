#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class FileContexts {

 public:
  void register_lines(const std::string &path, std::vector<std::string> lines) {
    contexts[path] = lines;
  }

  std::string get_line(const std::string &path, unsigned lineno) const {
    auto it = contexts.find(path);
    if (it == contexts.end())
      return "";
    if (lineno == 0 || lineno >= it->second.size())
      return "";
    return it->second[lineno - 1];
  }

 private:
  std::unordered_map<std::string, std::vector<std::string>> contexts;
};
