#pragma once

#include <clink/iter.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** create an iterator for parsing the given C/C++ file
 *
 * \param it [out] Created iterator on success
 * \param filename Path to source file to parse
 * \param argc Number of elements in argv
 * \param argv Arguments to pass to Clang
 * \returns 0 on success or an errno on failure
 */
int clink_parse_c(clink_iter_t **it, const char *filename, size_t argc,
  const char **argv);

#ifdef __cplusplus
}
#endif
