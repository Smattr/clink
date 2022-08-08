#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLINK_API
#define CLINK_API __attribute__((visibility("default")))
#endif

typedef enum {
  CLINK_DEFINITION = 0,
  CLINK_FUNCTION_CALL = 1,
  CLINK_REFERENCE = 2,
  CLINK_INCLUDE = 3,
} clink_category_t;

typedef struct {

  /// the type of this symbol
  clink_category_t category;

  /// name of this item or referent
  char *name;

  /// path to the containing file of this symbol
  char *path;

  /// location within the containing file
  unsigned long lineno;
  unsigned long colno;

  /// optional containing definition
  char *parent;

  /// optional content of the containing source line
  char *context;

} clink_symbol_t;

/** duplicate a symbol structure
 *
 * \param dst [out] Output copy on success
 * \param src Input symbol to copy
 * \return 0 on success or an errno on failure
 */
CLINK_API int clink_symbol_copy(clink_symbol_t *__restrict__ dst,
                                const clink_symbol_t *__restrict__ src);

/** clean up and deallocate the contents of a symbol structure
 *
 * \param s Symbol to clear
 */
CLINK_API void clink_symbol_clear(clink_symbol_t *s);

#ifdef __cplusplus
}
#endif
