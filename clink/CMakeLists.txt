add_executable(clink
  src/build.c
  src/colour.c
  src/dirname.c
  src/disppath.c
  src/file_queue.c
  src/help.c
  src/is_root.c
  src/join.c
  src/line_ui.c
  src/main.c
  src/mtime.c
  src/ncurses_ui.c
  src/option.c
  src/path.c
  src/set.c
  src/sigint.c
  src/str_queue.c
  src/work_queue.c
  ${CMAKE_CURRENT_BINARY_DIR}/manpage.c)

target_include_directories(clink PRIVATE src)

target_link_libraries(clink libclink)

find_package(Threads REQUIRED)
target_link_libraries(clink ${CMAKE_THREAD_LIBS_INIT})

set(CURSES_NEED_NCURSES True)
find_package(Curses REQUIRED)
target_include_directories(clink PUBLIC ${CURSES_INCLUDE_DIR})
target_link_libraries(clink ${CURSES_LIBRARIES})

find_program(XXD xxd)
add_custom_command(OUTPUT manpage.c
  COMMAND ${XXD} -include clink.1 ${CMAKE_CURRENT_BINARY_DIR}/manpage.c
  MAIN_DEPENDENCY src/clink.1
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_custom_target(man-clink
  ALL
  DEPENDS clink.1.gz)
add_custom_command(OUTPUT clink.1.gz
  COMMAND gzip -9 --no-name --to-stdout src/clink.1 >"${CMAKE_CURRENT_BINARY_DIR}/clink.1.gz"
  MAIN_DEPENDENCY src/clink.1
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS clink
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/clink.1.gz
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
