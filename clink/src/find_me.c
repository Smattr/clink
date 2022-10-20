#include "find_me.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#ifdef __FreeBSD__
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

/// read the given symlink in the /proc file system
static char *read_proc(const char *path) {

  char buf[PATH_MAX + 1] = {0};
  if (readlink(path, buf, sizeof(buf)) < 0)
    return NULL;

  // was the path too long?
  if (buf[sizeof(buf) - 1] != '\0')
    return NULL;

  return strdup(buf);
}

char *find_me(void) {

  // Linux
  char *path = read_proc("/proc/self/exe");
  if (path != NULL)
    return path;

  // DragonFly BSD, FreeBSD
  path = read_proc("/proc/curproc/file");
  if (path != NULL)
    return path;

  // NetBSD
  path = read_proc("/proc/curproc/exe");
  if (path != NULL)
    return path;

// /proc-less FreeBSD
#ifdef __FreeBSD__
  {
    int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
    static const size_t MIB_LENGTH = sizeof(mib) / sizeof(mib[0]);
    char buf[PATH_MAX + 1] = {0};
    size_t buf_size = sizeof(buf);
    if (sysctl(mib, MIB_LENGTH, buf, &buf_size, NULL, 0) == 0)
      return strdup(buf);
  }
#endif

  // macOS
#ifdef __APPLE__
  {
    // determine how many bytes we will need to allocate
    uint32_t buf_size = 0;
    int rc __attribute__((unused)) = _NSGetExecutablePath(NULL, &buf_size);
    assert(rc != 0);
    assert(buf_size > 0);

    path = malloc(buf_size);
    if (path == NULL)
      return NULL;

    // retrieve the actual path
    if (_NSGetExecutablePath(path, &buf_size) < 0) {
      free(path);
      return NULL;
    }

    // try to resolve any levels of symlinks if possible
    while (true) {
      char buf[PATH_MAX + 1] = {0};
      if (readlink(path, buf, sizeof(buf)) < 0)
        return path;

      free(path);
      path = strdup(buf);
      if (path == NULL)
        return NULL;
    }
  }
#endif

  return NULL;
}
