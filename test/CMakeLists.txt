add_subdirectory(ls-includes)
add_subdirectory(vim-open)

add_executable(unit-tests
  main.c
  db_add_line/main.c
  db_add_record/main.c
  db_add_symbol/main.c
  db_add_symbol_no_parent/main.c
  db_find_call/main.c
  db_find_caller/main.c
  db_find_definition/main.c
  db_find_file/main.c
  db_find_includer/main.c
  db_find_includer_stem/main.c
  db_find_record/main.c
  db_find_symbol/main.c
  db_open/main.c
  db_remove/main.c
  db_remove_empty/main.c
  dirname/main.c
  ../../clink/src/dirname.c
  disppath/main.c
  ../../clink/src/disppath.c
  ../../clink/src/path.c
  is_root/main.c
  ../../clink/src/is_root.c
  join/main.c
  ../../clink/src/join.c
  run-echo/main.c
  ../../libclink/src/get_environ.c
  ../../libclink/src/run.c
  run-touch/main.c
)

target_link_libraries(unit-tests PRIVATE libclink)

add_custom_target(check
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/unit-tests
)

add_dependencies(check unit-tests)
