add_executable(clink
  src/build.cc
  src/colours.cc
  src/ends_with.cc
  src/get_db_path.cc
  src/get_mtime.cc
  src/line_ui.cc
  src/lstrip.cc
  src/main.cc
  src/Options.cc
  src/ParseAsm.cc
  src/ParseC.cc
  src/ReadFile.cc
  src/UICurses.cc
  src/WorkQueue.cc)

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
