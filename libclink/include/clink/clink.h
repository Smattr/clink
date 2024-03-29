#pragma once

#ifndef CLINK_API
#define CLINK_API __attribute__((visibility("hidden")))
#endif

#include <clink/asm.h>
#include <clink/c.h>
#include <clink/clang.h>
#include <clink/cscope.h>
#include <clink/db.h>
#include <clink/debug.h>
#include <clink/def.h>
#include <clink/generic.h>
#include <clink/iter.h>
#include <clink/python.h>
#include <clink/symbol.h>
#include <clink/tablegen.h>
#include <clink/version.h>
#include <clink/vim.h>
