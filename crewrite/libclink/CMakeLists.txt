add_library(clink
  src/asm.c
  src/c.c)

target_include_directories(clink
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>)

# find libclang
find_program(LLVM_CONFIG llvm-config)
if (NOT LLVM_CONFIG)
  message(FATAL_ERROR "could not find llvm-config")
endif ()
execute_process(COMMAND ${LLVM_CONFIG} --includedir
  OUTPUT_VARIABLE CLANG_INCDIR
  OUTPUT_STRIP_TRAILING_WHITESPACE)
target_include_directories(clink PUBLIC "${CLANG_INCDIR}")
execute_process(COMMAND ${LLVM_CONFIG} --libdir
  OUTPUT_VARIABLE CLANG_LIBDIR
  OUTPUT_STRIP_TRAILING_WHITESPACE)
find_library(CLANG_LIBRARY NAMES clang HINTS ${CLANG_LIBDIR})
if (NOT CLANG_LIBRARY)
  message(FATAL_ERROR "could not find libclang")
endif ()
target_link_libraries(clink ${CLANG_LIBRARY})

install(TARGETS clink EXPORT LibclinkConfig
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# test exe for ASM parsing
add_executable(parse-asm src/asm.c)
target_compile_definitions(parse-asm PRIVATE -DMAIN_ASM)
target_include_directories(parse-asm
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>)

# test exe for C parsing
add_executable(parse-c src/c.c)
target_compile_definitions(parse-c PRIVATE -DMAIN_C)
target_include_directories(parse-c
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>)
target_include_directories(parse-c PRIVATE "${CLANG_INCDIR}")
target_link_libraries(parse-c ${CLANG_LIBRARY})

export(TARGETS clink FILE LibclinkConfig.cmake)
