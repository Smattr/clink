#pragma once

#include "../../common/compiler.h"

/// translate a POSIX regex.h error to an errno
INTERNAL int re_err_to_errno(int err);
