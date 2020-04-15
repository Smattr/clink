add_executable(run-touch main.c ../../libclink/src/get_environ.c
  ../../libclink/src/run.c)
target_include_directories(run-touch PRIVATE ../../libclink/src)
