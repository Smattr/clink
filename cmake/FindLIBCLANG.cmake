find_program(LLVM_CONFIG
  NAMES
    llvm-config
    llvm-config-mp-14 llvm-config-14
    llvm-config-mp-13 llvm-config-13
    llvm-config-mp-12 llvm-config-12
    llvm-config-mp-11 llvm-config-11
  REQUIRED)

execute_process(
  COMMAND ${LLVM_CONFIG} --includedir
  OUTPUT_VARIABLE LIBCLANG_INCLUDE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY)

execute_process(
  COMMAND ${LLVM_CONFIG} --libdir
  OUTPUT_VARIABLE LIBCLANG_LIBRARY_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY)

find_library(LIBCLANG_LIBRARY
  NAMES clang
  HINTS ${LIBCLANG_LIBRARY_DIR} ENV LIBRARY_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBCLANG
  REQUIRED_VARS LIBCLANG_INCLUDE_DIR LIBCLANG_LIBRARY)

set(LIBCLANG_INCLUDE_DIRS ${LIBCLANG_INCLUDE_DIR})
set(LIBCLANG_LIBRARIES ${LIBCLANG_LIBRARY})
