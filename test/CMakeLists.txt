add_subdirectory(ls-includes)
add_subdirectory(vim-open)

add_executable(unit-tests
  main.c
  db_add_line.c
  db_add_record.c
  db_add_symbol.c
  db_add_symbol_no_parent.c
  db_find_call.c
  db_find_caller.c
  db_find_definition.c
  db_find_file.c
  db_find_includer.c
  db_find_includer_stem.c
  db_find_record.c
  db_find_symbol.c
  db_open.c
  db_remove.c
  db_remove_empty.c
  dirname.c
  ../../clink/src/dirname.c
  disppath.c
  ../../clink/src/disppath.c
  ../../clink/src/path.c
  is_root.c
  ../../clink/src/is_root.c
  join.c
  ../../clink/src/join.c
  run-echo.c
  ../../libclink/src/get_environ.c
  ../../libclink/src/run.c
  run-touch.c
)

target_link_libraries(unit-tests PRIVATE libclink)

add_custom_target(check
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/unit-tests
)

add_dependencies(check unit-tests)
