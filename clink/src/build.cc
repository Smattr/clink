#include <cstddef>
#include "build.h"
#include <clink/clink.h>
#include "Task.h"
#include "WorkQueue.h"

using namespace clink;

static void update(Database &db, WorkQueue &q) {
  while (std::unique_ptr<Task> item = q.pop())
    item->run(db, q);
}

int build(Database &db, time_t era_start) {

  // create a queue of work to be done, initially containing just the current
  // directory
  WorkQueue q(".", era_start);

  // construct/update the database by scanning for changes since the last update
  update(db, q);

  return EXIT_SUCCESS;
}
