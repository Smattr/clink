#pragma once

#include <dirent.h>
#include <exception>
#include <mutex>
#include <stack>
#include <string>
#include <sys/types.h>
#include <tuple>
#include "WorkItem.h"

class NoMoreEntries : public std::exception {
    virtual const char *what() const noexcept {
        return "no more entries";
    }
};

class WorkQueue {

public:
    WorkQueue(const std::string &directory, time_t era_start);
    virtual WorkItem *pop();

private:
    time_t era_start;
    std::stack<std::tuple<std::string, DIR*>> directory_stack;

    bool push_directory_stack(const std::string &directory);
};

class ThreadSafeWorkQueue : public WorkQueue {

public:
    ThreadSafeWorkQueue(const std::string &directory, time_t era_start)
        : WorkQueue(directory, era_start) {
    }
    WorkItem *pop() final;

private:
    std::mutex stack_mutex;

};