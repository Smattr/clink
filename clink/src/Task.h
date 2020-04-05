#pragma once

#include <clink/clink.h>

// forward declaration so we can refer to this with a circular #include
class WorkQueue;

// abstract model of an unit of work to be done
class Task {

 public:
  // inheritors should implement this method to take whatever action is
  // appropriate to conceptually “do” their chunk of work
  virtual void run(clink::Database &db, WorkQueue &q) = 0;

  virtual ~Task() = default;
};
