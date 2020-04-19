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

find_path(CLANG_INCDIR clang-c/Index.h
  HINTS ENV C_INCLUDE_PATH)
target_include_directories(libclink PUBLIC ${CLANG_INCDIR})
find_library(CLANG_LIBRARIES clang
  HINTS ENV LIBRARY_PATH)
target_link_libraries(libclink ${CLANG_LIBRARIES})

find_package(SQLite3 REQUIRED)
target_include_directories(libclink PUBLIC ${SQLite3_INCLUDE_DIRS})
target_link_libraries(libclink ${SQLite3_LIBRARIES})

install(TARGETS libclink EXPORT LibclinkConfig
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

export(TARGETS libclink FILE LibclinkConfig.cmake)
