#include <cstddef>
#include "build.h"
#include <cassert>
#include <climits>
#include <clink/clink.h>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <errno.h>
#include <filesystem>
#include <functional>
#include "get_db_path.h"
#include "get_mtime.h"
#include <getopt.h>
#include <iostream>
#include "line_ui.h"
#include <memory>
#include "Options.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include "UICurses.h"
#include <unistd.h>

int main(int argc, char **argv) {

  parse_options(argc, argv);

  std::filesystem::path db_path = get_db_path();

  /* Stat the database to figure out when the last update we did was. */
  time_t era_start = get_mtime(db_path);

  std::unique_ptr<clink::Database> db;
  try {
    db = std::make_unique<clink::Database>(db_path.string());
  } catch (clink::Error &e) {
    std::cerr << "failed to open " << db_path << ": " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  int rc = EXIT_SUCCESS;

  if (options.update_database) {
    if ((rc = build(*db, era_start)) != EXIT_SUCCESS)
      return rc;
  }

  if (options.ncurses_ui) {
    UICurses ui;
    if ((rc = ui.run(*db)) != EXIT_SUCCESS)
      return rc;
  }

  if (options.line_ui) {
    if ((rc = line_ui(*db)) != EXIT_SUCCESS)
      return rc;
  }

  return EXIT_SUCCESS;
}
