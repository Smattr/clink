// generic target-agnostic assembly parser
//
// “but... isn't that impossible...?”
//
// Well strictly yes, in the sense that different platforms do not have a common
// assembly format. Even different assemblers do not have a common format.
// Instead of trying to cope directly with this variation, this parser works on
// the fuzzy notion that each line of an assembly file consists of one of the
// following forms:
//
//   1. #define FOO...
//   2. #include <...> or #include "..."
//   3. function_name:
//   4. branch_instruction target
//   5. something else irrelevant
//
// (1) and (2) give us enough pre-processor functionality for Clink’s needs. For
// (3), the rules for what can be a function name are essentially
// “[a-zA-Z_.][\w_.$@]*”. This isn't accurate, but matches most symbols humans
// come up with. For (4), the parser has a loose understanding of what branch
// instructions look like on several relevant platforms. Anything else that
// falls under (5) just gets ignored.
//
// The above sounds like quite a dumb approach, but you would be surprised how
// well it works.

#pragma once

#include <clink/iter.h>

#ifdef __cplusplus
extern "C" {
#endif

/** create an iterator for parsing the given assembly file
 *
 * \param it [out] Created iterator on success
 * \param filename Path to source file to parse
 * \returns 0 on success or an errno on failure
 */
int clink_parse_asm(clink_iter_t **it, const char *filename);

#ifdef __cplusplus
}
#endif
