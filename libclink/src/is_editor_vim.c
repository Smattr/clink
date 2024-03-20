#include "debug.h"
#include <assert.h>
#include <clink/vim.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/** does a path look like the given alias?
 *
 * \param base Start of the path to test
 * \param length Number of bytes in the path
 * \param alias An alias for Vim
 * \return True if this is a path to a binary of the given alias
 */
static bool is(const char *base, size_t length, const char *alias) {
  assert(base != NULL);
  assert(alias != NULL);

  const size_t alias_len = strlen(alias);

  if (alias_len > length)
    return false;

  if (strcmp(&base[length - alias_len], alias) != 0)
    return false;

  if (alias_len == length)
    return true;

  if (base[length - alias_len - 1] == '/')
    return true;

  return false;
}

bool clink_is_editor_vim(void) {

  // priority 1: `$VISUAL`
  const char *editor = getenv("VISUAL");

  // priority 2: `$EDITOR`
  if (editor == NULL)
    editor = getenv("EDITOR");

  // else fallback to Vim
  if (editor == NULL)
    editor = "vim";

  // `$EDITOR` can contain something like “vim -C”, so assume only the first
  // word is the actual path
  const char *space = strchr(editor, ' ');
  const size_t len = space == NULL ? strlen(editor) : (size_t)(space - editor);

  // aliases Vim goes by
  const char *VIM_NAMES[] = {"eview", "evim",  "ex",     "gex",   "gview",
                             "gvim",  "rview", "rgview", "rgvim", "rvim",
                             "vi",    "view",  "vim",    "vimx"};

  for (size_t i = 0; i < sizeof(VIM_NAMES) / sizeof(VIM_NAMES[0]); ++i) {
    if (is(editor, len, VIM_NAMES[i])) {
      DEBUG("editor \"%s\" matched \"%s\"", editor, VIM_NAMES[i]);
      return true;
    } else {
      DEBUG("editor \"%s\" did not match \"%s\"", editor, VIM_NAMES[i]);
    }
  }

  return false;
}
