#include <cassert>
#include <cstdio>
#include <cstdlib>
#include "FileReader.h"
#include <string>
#include <vector>
#include "Vim.h"

using namespace std;

static const Line NULL_LINE = { "", "" };

struct FileReaderImpl {
    FILE *fp;
    VimHighlighter highlighter;
    vector<Line> lines;
    char *lastline = nullptr;
    size_t lastline_size = 0;

    FileReaderImpl(const string &filename)
        : fp(fopen(filename.c_str(), "r")),
          highlighter(filename) {
    }

    ~FileReaderImpl() {
        if (fp != nullptr)
            fclose(fp);
        free(lastline);
    }
};

FileReader::FileReader(const string &filename) {
    impl = new FileReaderImpl(filename);
}

FileReader::~FileReader() {
    delete impl;
}

bool FileReader::is_ok() {
    return impl->fp != nullptr;
}

const Line &FileReader::get_line(unsigned lineno) {

    while (impl->lines.size() < lineno && is_ok()) {

        // Read the plain line
        assert(impl->fp != nullptr);
        if (getline(&impl->lastline, &impl->lastline_size, impl->fp) == -1) {
            fclose(impl->fp);
            impl->fp = nullptr;
            break;
        }

        // Read the highlighted line
        string highlighted = impl->highlighter.get_line(lineno);

        // Save it for later
        Line l { .plain = impl->lastline, .highlighted = highlighted };
        impl->lines.push_back(l);
    }

    if (lineno <= impl->lines.size() && lineno != 0)
        return impl->lines[lineno - 1];

    return NULL_LINE;
}
