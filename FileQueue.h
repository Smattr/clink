#pragma once

#include <dirent.h>
#include <exception>
#include <mutex>
#include <stack>
#include <string>
#include <sys/types.h>
#include <tuple>

class NoMoreEntries : public std::exception {
    virtual const char *what() const noexcept {
        return "no more entries";
    }
};

class FileQueue {

public:
    FileQueue(const std::string &directory, time_t era_start);
    virtual std::string pop();

private:
    time_t era_start;
    std::stack<std::tuple<std::string, DIR*>> directory_stack;

    bool push_directory_stack(const std::string &directory);
};

class ThreadSafeFileQueue : public FileQueue {

public:
    ThreadSafeFileQueue(const std::string &directory, time_t era_start)
        : FileQueue(directory, era_start) {
    }
    std::string pop() override;

private:
    std::mutex stack_mutex;

};
