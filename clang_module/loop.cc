#include <stdio.h>

__attribute__ ((noinline)) void DefaultImplementation() {
  printf("Default\n");
}

void TestLoop(int n) {
  for (int i = 0; i < n; ++i) {
    DefaultImplementation();
  }
}

