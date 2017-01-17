#include <cstring>
#include <dirent.h>
#include "FileQueue.h"
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <tuple>
#include <unistd.h>

using namespace std;

static bool ends_with(const char *s, const char *suffix) {
    size_t s_len = strlen(s);
    size_t suffix_len = strlen(suffix);
    return s_len >= suffix_len && strcmp(&s[s_len - suffix_len], suffix) == 0;
}

FileQueue::FileQueue(const string &directory, time_t era_start)
        : era_start(era_start) {

    string prefix = directory + "/";
    DIR *dir = opendir(prefix.c_str());
    if (dir != nullptr)
        directory_stack.push(make_tuple(prefix, dir));
}

bool FileQueue::push_directory_stack(const string &directory) {

    DIR *dir = opendir(directory.c_str());
    if (dir == nullptr) {
        // Failed to open the new directory. Just discard it.
        return false;
    }

    directory_stack.push(make_tuple(directory, dir));
    return true;
}

string FileQueue::pop() {

restart1:
    if (directory_stack.empty()) {
        throw NoMoreEntries();
    }

restart2:;
    DIR *current;
    string prefix;
    tie(prefix, current) = directory_stack.top();

    for (;;) {
        struct dirent entry, *result;
        if (readdir_r(current, &entry, &result) != 0 || result == nullptr) {
            // Exhausted this directory.
            closedir(current);
            current = nullptr;
            directory_stack.pop();
            goto restart1;
        }

        // If this is a directory, descend into it.
        if (entry.d_type == DT_DIR && strcmp(entry.d_name, ".") &&
                strcmp(entry.d_name, "..")) {
            string dname = prefix + entry.d_name + "/";
            push_directory_stack(dname);
            goto restart2;
        }

        // If this entry is a C/C++ file, see if it is "new".
        if (entry.d_type == DT_REG && (ends_with(entry.d_name, ".c") ||
                                       ends_with(entry.d_name, ".cpp") ||
                                       ends_with(entry.d_name, ".h") ||
                                       ends_with(entry.d_name, ".hpp"))) {
            string path = prefix + entry.d_name;
            struct stat buf;
            if (stat(path.c_str(), &buf) < 0 || buf.st_mtime <= era_start) {
                // Consider this file "old".
                continue;
            }
            return path;
        }

        // If we reached here, the directory entry was irrelevant to us.
    }
}
