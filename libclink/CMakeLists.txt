project(libclink LANGUAGES C)

add_library(libclink
  src/colour.c
  src/compiler_includes.c
  src/db_add_symbol.c
  src/db_close.c
  src/db_open.c
  src/db_remove.c
  src/get_environ.c
  src/iter.c
  src/parse_asm.c
  src/parse_c.c
  src/re.c
  src/run.c
  src/sql.c
  src/symbol.c
  src/temp_dir.c
  src/vim_highlight.c
  src/vim_open.c)

target_include_directories(libclink
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE
  src)

set_target_properties(libclink PROPERTIES PREFIX "")

find_program(LLVM_CONFIG llvm-config)
if(NOT LLVM_CONFIG)
  message(FATAL_ERROR "could not find llvm-config")
endif ()
execute_process(COMMAND ${LLVM_CONFIG} --includedir
  OUTPUT_VARIABLE CLANG_INCDIR
  OUTPUT_STRIP_TRAILING_WHITESPACE)
target_include_directories(libclink PUBLIC "${CLANG_INCDIR}")
execute_process(COMMAND ${LLVM_CONFIG} --libdir
  OUTPUT_VARIABLE CLANG_LIBDIR
  OUTPUT_STRIP_TRAILING_WHITESPACE)
find_library(CLANG_LIBRARY NAMES clang HINTS ${CLANG_LIBDIR})
if(NOT CLANG_LIBRARY)
  message(FATAL_ERROR "could not find libclang")
endif ()
target_link_libraries(libclink ${CLANG_LIBRARY})
target_link_options(libclink PUBLIC "-Wl,-rpath,${CLANG_LIBDIR}")

find_package(SQLite3 REQUIRED)
target_include_directories(libclink PUBLIC ${SQLite3_INCLUDE_DIRS})
target_link_libraries(libclink ${SQLite3_LIBRARIES})

install(TARGETS libclink EXPORT LibclinkConfig
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

export(TARGETS libclink FILE LibclinkConfig.cmake)
