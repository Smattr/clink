project(libclink LANGUAGES CXX)

add_library(libclink
  src/colour.cc
  src/Error.cc
  src/parse_asm.cc
  src/parse_c.cc
  src/run.cc
  src/vim_open.cc)

target_include_directories(libclink
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE
  src)

set_target_properties(libclink PROPERTIES PREFIX "")

find_program(LLVM_CONFIG llvm-config)
if (NOT LLVM_CONFIG)
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
if (NOT CLANG_LIBRARY)
  message(FATAL_ERROR "could not find libclang")
endif ()
target_link_libraries(libclink ${CLANG_LIBRARY})

install(TARGETS libclink EXPORT LibclinkConfig
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

export(TARGETS libclink FILE LibclinkConfig.cmake)
