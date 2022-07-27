/// \file
/// \brief common ctype.h-like functions

#pragma once

#include <ctype.h>
#include <stdbool.h>

/// is this an identifier starter?
static inline bool isid0(int c) { return isalpha(c) || c == '_'; }

/// is this an identifier continuer?
static inline bool isid(int c) { return isid0(c) || isdigit(c); }
