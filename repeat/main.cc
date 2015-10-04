#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: repeat <command>\n");
    return 1;
  }

  while (1) {
    pid_t child_pid = fork();
    if (child_pid == 0) {
      return execvp(argv[1], &argv[1]);
      // Child
    } else {
      // Parent
      int child_result;
      waitpid(child_pid, &child_result, 0);
      /*
      if (child_result != 0) {
        printf("Error in child: %d\n", child_result);
        return 1;
      }
      */
    }
  }

  return 0;
}
