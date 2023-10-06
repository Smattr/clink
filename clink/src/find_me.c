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

/// `readlink`-alike that dynamically allocates
static char *readln(const char *path) {

  char *resolved = NULL;
  size_t size = 512;

  while (true) {

    // expand target buffer
    size *= 2;
    {
      char *r = realloc(resolved, size);
      if (r == NULL)
        break;
      resolved = r;
    }

    // attempt to resolve
    {
      ssize_t written = readlink(path, resolved, size);
      if (written < 0)
        break;
      if ((size_t)written < size) {
        // success
        resolved[written] = '\0';
        return resolved;
      }
    }
  }

  // failed
  free(resolved);
  return NULL;
}

char *find_me(void) {

  // Linux
  char *path = readln("/proc/self/exe");
  if (path != NULL)
    return path;

  // DragonFly BSD, FreeBSD
  path = readln("/proc/curproc/file");
  if (path != NULL)
    return path;

  // NetBSD
  path = readln("/proc/curproc/exe");
  if (path != NULL)
    return path;

// /proc-less FreeBSD
#ifdef __FreeBSD__
  {
    int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
    static const size_t MIB_LENGTH = sizeof(mib) / sizeof(mib[0]);

    do {
      // determine how long the path is
      size_t buf_size = 0;
      if (sysctl(mib, MIB_LENGTH, NULL, &buf_size, NULL, 0) < 0)
        break;
      assert(buf_size > 0);

      // make enough space for the target path
      char *buf = malloc(buf_size);
      if (buf == NULL)
        break;

      // resolve it
      if (sysctl(mib, MIB_LENGTH, buf, &buf_size, NULL, 0) == 0)
        return buf;
      free(buf);
    } while (0);
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
      char *buf = readln(path);
      if (buf == NULL)
        return path;

      free(path);
      path = buf;
    }
  }
#endif

  return NULL;
}
