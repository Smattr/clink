project(libclink LANGUAGES C)

add_subdirectory(vimcat/libvimcat)

add_library(libclink
  src/compiler_includes.c
  src/db_add_line.c
  src/db_add_record.c
  src/db_add_symbol.c
  src/db_close.c
  src/db_find_call.c
  src/db_find_caller.c
  src/db_find_definition.c
  src/db_find_includer.c
  src/db_find_record.c
  src/db_find_symbol.c
  src/db_get_content.c
  src/db_open.c
  src/db_remove.c
  src/debug.c
  src/get_environ.c
  src/iter.c
  src/mmap.c
  src/parse_asm.c
  src/parse_def.c
  src/parse_generic.c
  src/parse_c.c
  src/parse_cpp.c
  src/parse_cxx.c
  src/parse_python.c
  src/parse_with_clang.c
  src/re.c
  src/re_sqlite.c
  src/run.c
  src/scanner.c
  src/sql.c
  src/symbol.c
  src/version_info.c
  src/vim_open.c
  src/vim_read.c
  src/vim_read_into.c
  ${CMAKE_CURRENT_BINARY_DIR}/version.c)

if(APPLE)
  target_compile_options(libclink PRIVATE -fno-common)
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_custom_command(
  OUTPUT version.c
  COMMAND src/make-version.py ${CMAKE_CURRENT_BINARY_DIR}/version.c
  MAIN_DEPENDENCY src/make-version.py
  DEPENDS always_run
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# dummy output to make sure we always re-evaluate the version step above
add_custom_command(
  OUTPUT always_run
  COMMAND /usr/bin/env true)

target_include_directories(libclink
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE
  src)

set_target_properties(libclink PROPERTIES PREFIX "")

target_link_libraries(libclink PRIVATE libvimcat)

find_package(LIBCLANG REQUIRED)
target_compile_definitions(libclink
  PRIVATE -DLLVM_VERSION="${LIBCLANG_LLVM_VERSION}")
target_include_directories(libclink SYSTEM PRIVATE ${LIBCLANG_INCLUDE_DIRS})
target_link_libraries(libclink PRIVATE ${LIBCLANG_LIBRARIES})

find_package(SQLite3 REQUIRED)
target_include_directories(libclink SYSTEM PRIVATE ${SQLite3_INCLUDE_DIRS})
target_link_libraries(libclink PRIVATE ${SQLite3_LIBRARIES})

install(TARGETS libclink EXPORT LibclinkConfig
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

export(TARGETS libclink FILE LibclinkConfig.cmake)
