add_executable(run-touch main.c ../../libclink/src/run.c)
target_include_directories(run-touch PRIVATE ../../libclink/src)
