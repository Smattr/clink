/// \file
/// \brief wrappers for the egregiously long POSIX Spawn functions

#pragma once

#include "get_environ.h"
#include <assert.h>
#include <fcntl.h>
#include <spawn.h>
#include <stddef.h>

static inline int adddup2(posix_spawn_file_actions_t *file_actions, int fildes,
                          int newfildes) {
  assert(file_actions != NULL);
  assert(fildes >= 0);
  assert(newfildes >= 0);
  return posix_spawn_file_actions_adddup2(file_actions, fildes, newfildes);
}

static inline int addopen(posix_spawn_file_actions_t *file_actions, int fildes,
                          const char *path, int oflag) {
  assert(file_actions != NULL);
  assert(fildes >= 0);
  assert(path != NULL);
  assert(oflag == O_RDONLY || oflag == O_WRONLY || oflag == O_RDWR);
  return posix_spawn_file_actions_addopen(file_actions, fildes, path, oflag, 0);
}

static inline int fa_init(posix_spawn_file_actions_t *file_actions) {
  assert(file_actions != NULL);
  return posix_spawn_file_actions_init(file_actions);
}

static inline void fa_destroy(posix_spawn_file_actions_t *file_actions) {
  assert(file_actions != NULL);
  (void)posix_spawn_file_actions_destroy(file_actions);
}

static inline int spawn(pid_t *pid, const char **argv,
                        posix_spawn_file_actions_t *file_actions) {
  assert(pid != NULL);
  assert(argv != NULL);
  assert(argv[0] != NULL);
  assert(file_actions != NULL);
  char *const *args = (char *const *)argv;
  return posix_spawnp(pid, args[0], file_actions, NULL, args, get_environ());
}
