cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
project(clink LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# make asprintf(), mkostemp(), pipe2() prototypes visible
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_compile_definitions(_GNU_SOURCE)
endif()

# enable --as-needed, present on GNU ld on Linux, to minimise dependencies
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed")
endif()

if(APPLE)
  list(APPEND CMAKE_INSTALL_RPATH "@executable_path/../${CMAKE_INSTALL_LIBDIR}")
else()
  list(APPEND CMAKE_INSTALL_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()

# when building static libraries, at best the visibility annotations are
# redundant and at worst they are incorrect (`__declspec(dllimport)` on MSVC),
# so suppress them
if(NOT BUILD_SHARED_LIBS)
  add_compile_definitions(CLINK_API=)
endif()

# enable even more warnings if the compiler supports them
include(CheckCCompilerFlag)
CHECK_C_COMPILER_FLAG(-W HAS_WARNING_W)
if(HAS_WARNING_W)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W")
endif()
CHECK_C_COMPILER_FLAG(-Wall HAS_WARNING_ALL)
if(HAS_WARNING_ALL)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif()
CHECK_C_COMPILER_FLAG(-Wextra HAS_WARNING_EXTRA)
if(HAS_WARNING_EXTRA)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
endif()
CHECK_C_COMPILER_FLAG(-Wcast-align=strict HAS_WARNING_CAST_ALIGN_STRICT)
if(HAS_WARNING_CAST_ALIGN_STRICT)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-align=strict")
endif()
CHECK_C_COMPILER_FLAG(-Wformat=2 HAS_WARNING_FORMAT_2)
if(HAS_WARNING_FORMAT_2)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat=2")
endif()
CHECK_C_COMPILER_FLAG(-Wformat-overflow=2 HAS_WARNING_FORMAT_OVERFLOW_2)
if(HAS_WARNING_FORMAT_OVERFLOW_2)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat-overflow=2")
endif()
CHECK_C_COMPILER_FLAG(-Wlogical-op HAS_WARNING_LOGICAL_OP)
if(HAS_WARNING_LOGICAL_OP)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wlogical-op")
endif()
CHECK_C_COMPILER_FLAG(-Wmissing-prototypes HAS_WARNING_MISSING_PROTOTYPES)
if(HAS_WARNING_MISSING_PROTOTYPES)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-prototypes")
endif()
CHECK_C_COMPILER_FLAG(-Wstrict-aliasing=1 HAS_WARNING_STRICT_ALIASING_1)
if(HAS_WARNING_STRICT_ALIASING_1)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-aliasing=1")
endif()
CHECK_C_COMPILER_FLAG(-Wpointer-arith HAS_WARNING_POINTER_ARITH)
if(HAS_WARNING_POINTER_ARITH)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpointer-arith")
endif()
CHECK_C_COMPILER_FLAG(-Wshadow HAS_WARNING_SHADOW)
if(HAS_WARNING_SHADOW)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wshadow")
endif()
CHECK_C_COMPILER_FLAG(-Wundef HAS_WARNING_UNDEF)
if(HAS_WARNING_UNDEF)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wundef")
endif()
CHECK_C_COMPILER_FLAG(-Wwrite-strings HAS_WARNING_WRITE_STRINGS)
if(HAS_WARNING_WRITE_STRINGS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wwrite-strings")
endif()

# enable LTO in release builds
include(CheckIPOSupported)
check_ipo_supported(RESULT HAVE_IPO)
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR
   CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  if(HAVE_IPO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  endif()
endif()

add_subdirectory(clink)
add_subdirectory(libclink)
add_subdirectory(test)

find_program(CLANG_FORMAT
  NAMES
    clang-format
    clang-format-mp-20 clang-format-20
    clang-format-mp-19 clang-format-19
    clang-format-mp-18 clang-format-18
    clang-format-mp-17 clang-format-17
    clang-format-mp-16 clang-format-16
    clang-format-mp-15 clang-format-15
    clang-format-mp-14 clang-format-14
    clang-format-mp-13 clang-format-13
    clang-format-mp-12 clang-format-12
    clang-format-mp-11 clang-format-11)
find_program(GIT git)
find_program(XARGS xargs)
if(CLANG_FORMAT AND GIT AND XARGS)
  add_custom_target(clang-format
    COMMAND ${GIT} ls-files -z -- ':!:test/cases' '**/*.c' '**/*.h' |
      ${XARGS} -0 -- ${CLANG_FORMAT} -i --style=file
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "clang-formatting C sources")
endif()
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import black"
    RESULT_VARIABLE IMPORT_BLACK_RET
    OUTPUT_QUIET
    ERROR_QUIET
  )
  if(${IMPORT_BLACK_RET} EQUAL 0 AND GIT AND XARGS)
    add_custom_target(python-format
      COMMAND
        ${GIT} ls-files -z -- ':!:test/cases' '**/*.py' clink/src/clink-repl |
        ${XARGS} -0 -- ${Python3_EXECUTABLE} -m black
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "black-formatting Python sources")
  endif()
endif()
add_custom_target(format
  DEPENDS clang-format python-format
  COMMENT "formatting sources")
