add_executable(clink
  src/colours.cc
  src/Database.cc
  src/log.cc
  src/main.cc
  src/UICurses.cc
  src/UILine.cc
  src/Vim.cc
  src/WorkItem.cc
  src/WorkQueue.cc)

target_include_directories(clink PRIVATE src)

add_executable(vimcat
  src/colours.cc
  src/log.cc
  src/Vim.cc
  src/vimcat.cc)

target_include_directories(vimcat PRIVATE src)

target_link_libraries(clink libclink)

find_package(Threads REQUIRED)
target_link_libraries(clink ${CMAKE_THREAD_LIBS_INIT})

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
find_package(Curses REQUIRED)
target_include_directories(clink PUBLIC ${CURSES_INCLUDE_DIR})
target_link_libraries(clink ${CURSES_LIBRARIES})
target_include_directories(vimcat PUBLIC ${CURSES_INCLUDE_DIR})
target_link_libraries(vimcat ${CURSES_LIBRARIES})

find_package(SQLite3 REQUIRED)
target_include_directories(clink PUBLIC ${SQLite3_INCLUDE_DIRS})
target_link_libraries(clink ${SQLite3_LIBRARIES})

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

install(TARGETS clink
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS vimcat
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
