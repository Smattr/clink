// implementation of a string-containing set

#pragma once

typedef struct set set_t;

/** create a new string set
 *
 * \param s [out] An initialised set on success
 * \returns 0 on success or an errno on failure
 */
int set_new(set_t **s);

/** add a new string to the set
 *
 * \param s Set to operate on
 * \param item String to add
 * \returns 0 on success if the string was not already present in the set,
 *   EALREADY if the string was already in the set, or an errno on failure
 */
int set_add(set_t *s, const char *item);

/** clear and deallocate a string set
 *
 * \param s Set to destroy
 */
void set_free(set_t **s);
