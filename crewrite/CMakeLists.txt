cmake_minimum_required(VERSION 3.2)
project(clink C)

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Wextra -Wwrite-strings -Wmissing-declarations -Wshadow)

# enable --as-needed, present on GNU ld on Linux, to minimise dependencies
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed")
endif()

add_subdirectory(libclink)
