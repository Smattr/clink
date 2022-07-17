add_subdirectory(db_add_line)
add_subdirectory(db_add_record)
add_subdirectory(db_add_symbol)
add_subdirectory(db_add_symbol_no_parent)
add_subdirectory(db_find_call)
add_subdirectory(db_find_caller)
add_subdirectory(db_find_definition)
add_subdirectory(db_find_file)
add_subdirectory(db_find_includer)
add_subdirectory(db_find_includer_stem)
add_subdirectory(db_find_record)
add_subdirectory(db_find_symbol)
add_subdirectory(db_open)
add_subdirectory(db_remove)
add_subdirectory(db_remove_empty)
add_subdirectory(dirname)
add_subdirectory(disppath)
add_subdirectory(is_root)
add_subdirectory(join)
add_subdirectory(ls-includes)
add_subdirectory(run-echo)
add_subdirectory(run-touch)
add_subdirectory(vim-open)

add_custom_target(check
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/db_add_line/db_add_line
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/db_add_record/db_add_record
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/db_add_symbol/db_add_symbol
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/db_add_symbol_no_parent/db_add_symbol_no_parent
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/db_find_call/db_find_call
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/db_find_caller/db_find_caller
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/db_find_definition/db_find_definition
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/db_find_file/db_find_file
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/db_find_includer/db_find_includer
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/db_find_includer_stem/db_find_includer_stem
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/db_find_record/db_find_record
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/db_find_symbol/db_find_symbol
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/db_open/db_open
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/db_remove/db_remove
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/db_remove_empty/db_remove_empty
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/dirname/dirname
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/disppath/disppath
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/is_root/is_root
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/join/join
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run-echo/run-echo
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run-touch/run-touch
)

add_dependencies(check db_add_line)
add_dependencies(check db_add_record)
add_dependencies(check db_add_symbol)
add_dependencies(check db_add_symbol_no_parent)
add_dependencies(check db_find_call)
add_dependencies(check db_find_caller)
add_dependencies(check db_find_definition)
add_dependencies(check db_find_file)
add_dependencies(check db_find_includer)
add_dependencies(check db_find_includer_stem)
add_dependencies(check db_find_record)
add_dependencies(check db_find_symbol)
add_dependencies(check db_open)
add_dependencies(check db_remove)
add_dependencies(check db_remove_empty)
add_dependencies(check dirname)
add_dependencies(check disppath)
add_dependencies(check is_root)
add_dependencies(check join)
add_dependencies(check run-echo)
add_dependencies(check run-touch)
