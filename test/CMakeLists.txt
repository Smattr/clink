add_subdirectory(libclang-ls)
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
  ../../clink/src/cwd.c
  ../../clink/src/disppath.c
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

find_package(Python3 COMPONENTS Interpreter)
if(NOT Python3_FOUND)
  message(WARNING "python3 not found; disabling make check")
  return()
endif()

execute_process(
  COMMAND ${Python3_EXECUTABLE} -c "import pytest"
  RESULT_VARIABLE IMPORT_PYTEST_RET
  OUTPUT_QUIET
  ERROR_QUIET
)
if(NOT ${IMPORT_PYTEST_RET} EQUAL 0)
  message(WARNING "pytest not found; disabling make check")
  return()
endif()

find_program(SQLITE3 sqlite3)
if(NOT SQLITE3)
  message(WARNING "sqlite3 not found; disabling make check")
  return()
endif()

find_program(VIM vim)
if(NOT VIM)
  message(WARNING "vim not found; disabling make check")
  return()
endif()

add_custom_target(check
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/unit-tests
  COMMAND env PATH=${CMAKE_BINARY_DIR}/clink:$ENV{PATH}
    ${Python3_EXECUTABLE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/integration.py
    --verbose
)

add_dependencies(check clink unit-tests)
