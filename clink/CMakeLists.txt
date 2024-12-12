add_executable(clink
  src/build.c
  src/colour.c
  src/compile_commands_close.c
  src/compile_commands_find.c
  src/compile_commands_open.c
  src/cwd.c
  src/dirname.c
  src/disppath.c
  src/fdbuf_free.c
  src/fdbuf_new.c
  src/fdbuf_writeback.c
  src/file_queue.c
  src/find_me.c
  src/find_repl.c
  src/have_vim.c
  src/help.c
  src/highlight.c
  src/is_root.c
  src/join.c
  src/main.c
  src/ui.c
  src/option.c
  src/path.c
  src/progress.c
  src/re.c
  src/screen.c
  src/set.c
  src/sigint.c
  src/spinner.c
  src/str_queue.c
  ${CMAKE_CURRENT_BINARY_DIR}/manpage.c)

target_include_directories(clink PRIVATE src)

target_link_libraries(clink PRIVATE libclink)

find_package(Threads REQUIRED)
target_link_libraries(clink PRIVATE ${CMAKE_THREAD_LIBS_INIT})

find_package(LIBCLANG REQUIRED)
target_include_directories(clink SYSTEM PRIVATE ${LIBCLANG_INCLUDE_DIRS})
target_link_libraries(clink PRIVATE ${LIBCLANG_LIBRARIES})

find_package(SQLite3 REQUIRED)
target_include_directories(clink SYSTEM PRIVATE ${SQLite3_INCLUDE_DIRS})
target_link_libraries(clink PRIVATE ${SQLite3_LIBRARIES})

find_program(XXD xxd REQUIRED)
add_custom_command(OUTPUT manpage.c
  COMMAND ${XXD} -include clink.1 ${CMAKE_CURRENT_BINARY_DIR}/manpage.c
  MAIN_DEPENDENCY src/clink.1
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)

find_program(GZIP gzip REQUIRED)
add_custom_target(man-clink
  ALL
  DEPENDS clink.1.gz clink-repl.1.gz)
add_custom_command(OUTPUT clink.1.gz
  COMMAND ${GZIP} -9 --no-name --to-stdout src/clink.1
    >"${CMAKE_CURRENT_BINARY_DIR}/clink.1.gz"
  MAIN_DEPENDENCY src/clink.1
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_custom_command(OUTPUT clink-repl.1.gz
  COMMAND ${GZIP} -9 --no-name --to-stdout src/clink-repl.1
    >"${CMAKE_CURRENT_BINARY_DIR}/clink-repl.1.gz"
  MAIN_DEPENDENCY src/clink-repl.1
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS clink
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/clink.1.gz
  ${CMAKE_CURRENT_BINARY_DIR}/clink-repl.1.gz
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(PROGRAMS src/clink-repl
  DESTINATION ${CMAKE_INSTALL_BINDIR})
