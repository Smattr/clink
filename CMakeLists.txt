cmake_minimum_required(VERSION 2.6)
project(clink CXX)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -W -Wall -Wextra")

add_executable(clink  AsmParser.cpp CXXParser.cpp Database.cpp FileQueue.cpp
  main.cpp UICurses.cpp UILine.cpp Vim.cpp)

find_program(LLVM_CONFIG llvm-config)
if (NOT LLVM_CONFIG)
  message(FATAL_ERROR "could not find llvm-config")
endif ()
execute_process(COMMAND ${LLVM_CONFIG} --includedir
  OUTPUT_VARIABLE CLANG_INCDIR
  OUTPUT_STRIP_TRAILING_WHITESPACE)
target_include_directories(clink PUBLIC "${CLANG_INCDIR}")
execute_process(COMMAND ${LLVM_CONFIG} --libdir
  OUTPUT_VARIABLE CLANG_LIBDIR
  OUTPUT_STRIP_TRAILING_WHITESPACE)
find_library(CLANG_LIBRARY NAMES clang HINTS ${CLANG_LIBDIR})
if (NOT CLANG_LIBRARY)
  message(FATAL_ERROR "could not find libclang")
endif ()
target_link_libraries(clink ${CLANG_LIBRARY})

set(CURSES_NEED_NCURSES True)
find_package(Curses)
target_include_directories(clink PUBLIC ${CURSES_INCLUDE_DIR})
target_link_libraries(clink ${CURSES_LIBRARIES})

find_package(PkgConfig)

pkg_check_modules(SQLITE3 REQUIRED sqlite3)
target_include_directories(clink PUBLIC ${SQLITE3_INCLUDE_DIRS})
target_link_libraries(clink ${SQLITE3_LIBRARIES})

find_path(READLINE_INCDIR NAMES readline/readline.h)
if (NOT READLINE_INCDIR)
  message(FATAL_ERROR "could not find GNU readline")
endif ()
target_include_directories(clink PUBLIC ${READLINE_INCDIR})
find_library(READLINE_LIBRARY NAMES readline)
if (NOT READLINE_LIBRARY)
  message(FATAL_ERROR "could not find GNU readline")
endif ()
target_link_libraries(clink ${READLINE_LIBRARY})
