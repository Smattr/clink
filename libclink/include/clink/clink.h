#pragma once

#ifndef CLINK_API
#ifdef __GNUC__
#define CLINK_API /* nothing */
#elif defined(_MSC_VER)
#define CLINK_API __declspec(dllimport)
#else
#define CLINK_API /* nothing */
#endif
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
