add_executable(clink
  src/main.c
  src/option.c
  src/path.c
  src/set.c
  src/sigterm.c
  src/str_queue.c)

target_include_directories(clink PRIVATE src)

target_link_libraries(clink libclink)

find_package(Threads REQUIRED)
target_link_libraries(clink ${CMAKE_THREAD_LIBS_INIT})

set(CURSES_NEED_NCURSES True)
find_package(Curses REQUIRED)
target_include_directories(clink PUBLIC ${CURSES_INCLUDE_DIR})
target_link_libraries(clink ${CURSES_LIBRARIES})

install(TARGETS clink
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
