add_executable(clink
  src/build.c
  src/loi.c
  src/main.c
  src/options.c)

target_include_directories(clink PRIVATE src)

target_link_libraries(clink libclink)

find_package(Threads REQUIRED)
target_link_libraries(clink ${CMAKE_THREAD_LIBS_INIT})

set(CURSES_NEED_NCURSES True)
find_package(Curses REQUIRED)
target_include_directories(clink PUBLIC ${CURSES_INCLUDE_DIR})
target_link_libraries(clink ${CURSES_LIBRARIES})

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
