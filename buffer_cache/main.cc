#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int main(int argc, char** argv) {
  int num_blocks = 10000;
  int num_replicas = 3;
  int num_iterations = 20;

  if (argc == 4) {
    num_blocks = atoi(argv[1]);
    num_replicas = atoi(argv[2]);
    num_iterations = atoi(argv[3]);
  }

  srand(time(0));

  bool* blocks_used = new bool[num_replicas * num_blocks];
  memset(blocks_used, 0, sizeof(bool) * num_replicas * num_blocks);

  for (int iter = 0; iter < num_iterations; ++iter) {
    for (int n = 0; n < num_blocks; ++n) {
      int replica = rand() % num_replicas;
      blocks_used[n * num_replicas + replica] = true;
    }
  }

  int num_used = 0;
  for (int i = 0; i < num_replicas * num_blocks; ++i) {
    if (blocks_used[i]) ++num_used;
  }

  printf("Total blocks: %d\n", num_blocks * num_replicas);
  printf("Blocks used: %d\n", num_used);
  printf("Percent: %f\n", num_used / (float) (num_blocks * num_replicas));

  return 0;
}
