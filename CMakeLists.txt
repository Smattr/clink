cmake_minimum_required(VERSION 3.14)
project(clink LANGUAGES C)

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wwrite-strings -Wmissing-prototypes -Wshadow")

# make asprintf() and posix_openpt() prototypes visible
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  add_definitions(-D_GNU_SOURCE -D_XOPEN_SOURCE=700)
endif()

# enable --as-needed, present on GNU ld on Linux, to minimise dependencies
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed")
endif()

# enable even more warnings if the compiler supports them
include(CheckCCompilerFlag)
CHECK_C_COMPILER_FLAG(-Wcast-align=strict HAS_WARNING_CAST_ALIGN_STRICT)
if(HAS_WARNING_CAST_ALIGN_STRICT)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-align=strict")
endif()
CHECK_C_COMPILER_FLAG(-Wstrict-aliasing=1 HAS_WARNING_STRICT_ALIASING_1)
if(HAS_WARNING_STRICT_ALIASING_1)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-aliasing=1")
endif()
CHECK_C_COMPILER_FLAG(-Wpointer-arith HAS_WARNING_POINTER_ARITH)
if(HAS_WARNING_POINTER_ARITH)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpointer-arith")
endif()

add_subdirectory(clink)
add_subdirectory(libclink)
add_subdirectory(vim-cat)

add_subdirectory(test/db_add_symbol)
add_subdirectory(test/db_add_symbol_no_parent)
add_subdirectory(test/db_open)
add_subdirectory(test/db_remove)
add_subdirectory(test/db_remove_empty)
add_subdirectory(test/html_colour_to_ansi)
add_subdirectory(test/ls-includes)
add_subdirectory(test/parse-asm)
add_subdirectory(test/parse-c)
add_subdirectory(test/run-echo)
add_subdirectory(test/run-echo-mask)
add_subdirectory(test/run-touch)
add_subdirectory(test/vim-open)

enable_testing()
add_test(db_add_symbol test/db_add_symbol/db_add_symbol)
add_test(db_add_symbol_no_parent
  test/db_add_symbol_no_parent/db_add_symbol_no_parent)
add_test(db_open test/db_open/db_open)
add_test(db_remove test/db_remove/db_remove)
add_test(db_remove_empty test/db_remove_empty/db_remove_empty)
add_test(html_colour_to_ansi test/html_colour_to_ansi/html_colour_to_ansi)
add_test(run-echo test/run-echo/run-echo)
add_test(run-echo-mask test/run-echo-mask/run-echo-mask)
add_test(run-touch test/run-touch/run-touch)
