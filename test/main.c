#include "test.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

test_case_t *test_cases;

bool has_assertion_;

int main(int argc, char **argv) {

  if (argc == 2 &&
      (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
    fprintf(stderr, "usage %s [test prefix]\n\ntest options:\n", argv[0]);
    for (test_case_t *p = test_cases; p != NULL; p = p->next) {
      fprintf(stderr, " %s\n", p->description);
    }
    return EXIT_FAILURE;
  }

  printf("Clink test suite\n");

  unsigned total = 0;
  unsigned failed = 0;
  for (test_case_t *p = test_cases; p != NULL; p = p->next) {
    if (argc == 1 || strncmp(argv[1], p->description, strlen(argv[1])) == 0) {

      printf("  Test: %s ... ", p->description);
      fflush(stdout);

      pid_t pid = fork();
      if (pid < 0) {
        fprintf(stderr, "fork failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
      }

      if (pid == 0) {
        p->function();
        run_cleanups();
        if (!has_assertion_) {
          fprintf(stderr, "failed\n    no assertions were executed\n");
          fflush(stderr);
          abort();
        }
        printf("OK\n");
        exit(EXIT_SUCCESS);

      } else {
        int status;
        (void)waitpid(pid, &status, 0);

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
          ++failed;
      }

      ++total;
    }
  }

  if (total == 0) {
    fprintf(stderr, "no tests found\n");
    return EXIT_FAILURE;
  }

  printf("%u/%u passed\n", total - failed, total);

  return failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
