add_executable(libclang-ls main.c)

find_package(LIBCLANG REQUIRED)
target_include_directories(libclang-ls SYSTEM PRIVATE ${LIBCLANG_INCLUDE_DIRS})
target_link_libraries(libclang-ls PRIVATE ${LIBCLANG_LIBRARIES})
