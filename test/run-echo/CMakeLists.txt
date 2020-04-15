add_executable(run-echo main.c ../../libclink/src/get_environ.c
  ../../libclink/src/run.c)
target_include_directories(run-echo PRIVATE ../../libclink/src)
