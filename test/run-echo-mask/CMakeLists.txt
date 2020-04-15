add_executable(run-echo-mask main.c ../../libclink/src/get_environ.c
  ../../libclink/src/run.c)
target_include_directories(run-echo-mask PRIVATE ../../libclink/src)
