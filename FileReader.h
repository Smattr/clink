/* Encapsulation for reading lines of a file as well as the Vim-highlighted
 * version of these lines.
 */

#pragma once

#include <string>

struct Line {
    const std::string plain;
    const std::string highlighted;
};

struct FileReaderImpl;

class FileReader {

public:
    FileReader(const std::string &filename);
    virtual ~FileReader();

    // Check whether the class is in a usable state.
    bool is_ok();

    const Line &get_line(unsigned lineno);

private:
    FileReaderImpl *impl;
};
