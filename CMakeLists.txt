cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
project(clink LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wwrite-strings -Wmissing-prototypes -Wshadow")

# make asprintf() prototype visible
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  add_definitions(-D_GNU_SOURCE)
endif()

# enable --as-needed, present on GNU ld on Linux, to minimise dependencies
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed")
endif()

# enable even more warnings if the compiler supports them
include(CheckCCompilerFlag)
CHECK_C_COMPILER_FLAG(-Wcast-align=strict HAS_WARNING_CAST_ALIGN_STRICT)
if(HAS_WARNING_CAST_ALIGN_STRICT)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-align=strict")
endif()
CHECK_C_COMPILER_FLAG(-Wstrict-aliasing=1 HAS_WARNING_STRICT_ALIASING_1)
if(HAS_WARNING_STRICT_ALIASING_1)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-aliasing=1")
endif()
CHECK_C_COMPILER_FLAG(-Wpointer-arith HAS_WARNING_POINTER_ARITH)
if(HAS_WARNING_POINTER_ARITH)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpointer-arith")
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
    COMMENT "clang-formatting sources")
endif()
add_custom_target(format
  DEPENDS clang-format
  COMMENT "formatting sources")
