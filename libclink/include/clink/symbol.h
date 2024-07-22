#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLINK_API
#ifdef __GNUC__
#define CLINK_API __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#define CLINK_API __declspec(dllexport)
#else
#define CLINK_API /* nothing */
#endif
#endif

typedef enum {
  CLINK_DEFINITION = 0,
  CLINK_FUNCTION_CALL = 1,
  CLINK_REFERENCE = 2,
  CLINK_INCLUDE = 3,
} clink_category_t;

/// a point within a source file
typedef struct {
  unsigned long lineno; ///< line number
  unsigned long colno;  ///< column number
  unsigned long byte;   ///< byte offset from the start of the containing file
} clink_location_t;

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

  /// range of containing semantic entity within the containing file
  ///
  /// When using a sophisticated parser like libclang, in contrast to `lineno`
  /// and `colno`, these fields typically cover more than just the symbol
  /// itself. E.g.:
  ///
  ///   int foo;
  ///   ▲   ▲ ▲
  ///   │   │ └─ `end`
  ///   │   └─ `lineno`, `colno`
  ///   └─ `start`
  ///
  /// If this precise information is unavailable (e.g. when using the
  /// Cscope-based parser), these fields will be 0.
  clink_location_t start;
  clink_location_t end;

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
