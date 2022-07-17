cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
project(clink LANGUAGES C)

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wwrite-strings -Wmissing-prototypes -Wshadow")

# make asprintf() prototype visible
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  add_definitions(-D_GNU_SOURCE)
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

add_subdirectory(test/db_add_line)
add_subdirectory(test/db_add_record)
add_subdirectory(test/db_add_symbol)
add_subdirectory(test/db_add_symbol_no_parent)
add_subdirectory(test/db_find_call)
add_subdirectory(test/db_find_caller)
add_subdirectory(test/db_find_definition)
add_subdirectory(test/db_find_file)
add_subdirectory(test/db_find_includer)
add_subdirectory(test/db_find_includer_stem)
add_subdirectory(test/db_find_record)
add_subdirectory(test/db_find_symbol)
add_subdirectory(test/db_open)
add_subdirectory(test/db_remove)
add_subdirectory(test/db_remove_empty)
add_subdirectory(test/dirname)
add_subdirectory(test/disppath)
add_subdirectory(test/is_root)
add_subdirectory(test/join)
add_subdirectory(test/ls-includes)
add_subdirectory(test/run-echo)
add_subdirectory(test/run-touch)
add_subdirectory(test/vim-open)

enable_testing()
add_test(db_add_line test/db_add_line/db_add_line)
add_test(db_add_record test/db_add_record/db_add_record)
add_test(db_add_symbol test/db_add_symbol/db_add_symbol)
add_test(db_add_symbol_no_parent
  test/db_add_symbol_no_parent/db_add_symbol_no_parent)
add_test(db_find_call test/db_find_call/db_find_call)
add_test(db_find_caller test/db_find_caller/db_find_caller)
add_test(db_find_definition test/db_find_definition/db_find_definition)
add_test(db_find_file test/db_find_file/db_find_file)
add_test(db_find_includer test/db_find_includer/db_find_includer)
add_test(db_find_includer_stem test/db_find_includer_stem/db_find_includer_stem)
add_test(db_find_record test/db_find_record/db_find_record)
add_test(db_find_symbol test/db_find_symbol/db_find_symbol)
add_test(db_open test/db_open/db_open)
add_test(db_remove test/db_remove/db_remove)
add_test(db_remove_empty test/db_remove_empty/db_remove_empty)
add_test(dirname test/dirname/dirname)
add_test(disppath test/disppath/disppath)
add_test(is_root test/is_root/is_root)
add_test(join test/join/join)
add_test(run-echo test/run-echo/run-echo)
add_test(run-touch test/run-touch/run-touch)

find_program(CLANG_FORMAT
  NAMES
    clang-format
    clang-format-mp-14 clang-format-14
    clang-format-mp-13 clang-format-13
    clang-format-mp-12 clang-format-12
    clang-format-mp-11 clang-format-11)
find_program(GIT git)
find_program(XARGS xargs)
if(CLANG_FORMAT AND GIT AND XARGS)
  add_custom_target(format
    COMMAND ${GIT} ls-files -z '**/*.c' '**/*.h' |
      ${XARGS} -0 -- ${CLANG_FORMAT} -i --style=file
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "clang-formatting sources")
endif()
