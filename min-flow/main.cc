#include <stdio.h>
#include <string.h>

#include <vector>
using namespace std;

const char* PEOPLE[] = {
  "Marshall",
  "Ariana",
  "Itay",
  "Nong",
  "Panda",
  "Mike",
  "Nisha",
  "Skye",
};

const int NUM_PEOPLE = sizeof(PEOPLE) / sizeof(PEOPLE[0]);

void printCsv(float* g) {
  for (int i = 0; i < NUM_PEOPLE; i++) {
    printf("%-10s", PEOPLE[i]);
    for (int j = 0; j < NUM_PEOPLE; j++) {
      if (g[i * NUM_PEOPLE + j] == 0) {
        printf("\t/");
      } else {
        printf("\t%0.2f", g[i * NUM_PEOPLE + j]);
      }
    }
    printf("\n");
  }
}

void printGraph(float* g, int w) {
  for (int i = 0; i < w; i++) {
    for (int j = 0; j < w; j++) {
      printf("%6.2f ", g[i * w + j]);
    }
    printf("\n");
  }
}

bool simplify(float* g, int w, int srcRow, int srcCol, int dstRow, int dstCol) {
  float v1 = g[srcCol * NUM_PEOPLE + dstCol];
  float v2 = g[dstCol * NUM_PEOPLE + srcCol];
  if (v1 != 0) {
    if (g[srcRow * NUM_PEOPLE + srcCol] < v1 && g[srcRow * NUM_PEOPLE + dstCol] != 0) {
      printGraph(g, w);
      printf("\n");
      g[srcCol * NUM_PEOPLE + dstCol] -= g[srcRow * NUM_PEOPLE + srcCol];
      g[srcRow * NUM_PEOPLE + dstCol] += g[srcRow * NUM_PEOPLE + srcCol];
      g[srcRow * NUM_PEOPLE + srcCol] = 0;
      return true;
    }
  } else if (v2 != 0) {
    if (g[dstRow * NUM_PEOPLE + dstCol] < v2 && g[srcRow * NUM_PEOPLE + srcCol] != 0) {
      g[dstCol * NUM_PEOPLE + srcCol] -= g[dstRow * NUM_PEOPLE + dstCol];
      g[srcRow * NUM_PEOPLE + srcCol] += g[dstRow * NUM_PEOPLE + dstCol];
      g[dstRow * NUM_PEOPLE + dstCol] = 0;
      return true;
    }
  }

  return false;
}

bool simplify(float* g, int w) {
  int start = -1;
  int end = -1;

  for (int i = 0; i < w; i++) {
    for (int c = 0; c < w; c++) {
      if (g[i * w + c] == 0) continue;

      int start = i * w + c;
      for (int owes = 0; owes < w; owes++) {
        if (owes != c && g[i * w + owes] != 0) {
          simplify(g, w, i, c, i, owes);
        }
      }
    }
  }
  return true;
}

int main(int argc, char** argv) {
  float* graph = new float[NUM_PEOPLE * NUM_PEOPLE];
  memset(graph, 0, sizeof(float) * NUM_PEOPLE * NUM_PEOPLE);
  graph[0 * NUM_PEOPLE + 2] = 381.81;
  graph[0 * NUM_PEOPLE + 4] = 83.46;

  graph[1 * NUM_PEOPLE + 0] = 231.75;
  graph[1 * NUM_PEOPLE + 2] = 467.09;
  graph[1 * NUM_PEOPLE + 3] = 30.29;
  graph[1 * NUM_PEOPLE + 4] = 315.21;
  graph[1 * NUM_PEOPLE + 5] = 6.61;
  graph[1 * NUM_PEOPLE + 7] = 36.26;

  graph[3 * NUM_PEOPLE + 0] = 198.96;
  graph[3 * NUM_PEOPLE + 2] = 467.49;
  graph[3 * NUM_PEOPLE + 4] = 282.43;

  graph[4 * NUM_PEOPLE + 2] = 0;

  graph[5 * NUM_PEOPLE + 0] = 235.47;
  graph[5 * NUM_PEOPLE + 2] = 460.49;
  graph[5 * NUM_PEOPLE + 3] = 23.68;
  graph[5 * NUM_PEOPLE + 4] = 283.08;
  graph[5 * NUM_PEOPLE + 7] = 23.43;

  graph[6 * NUM_PEOPLE + 0] = 75.61;
  graph[6 * NUM_PEOPLE + 2] = 467.09;
  graph[6 * NUM_PEOPLE + 3] = 30.29;
  graph[6 * NUM_PEOPLE + 4] = 203.92;
  graph[6 * NUM_PEOPLE + 5] = 6.61;
  graph[6 * NUM_PEOPLE + 7] = 30.04;

  graph[7 * NUM_PEOPLE + 0] = 207.08;
  graph[7 * NUM_PEOPLE + 2] = 422.43;
  graph[7 * NUM_PEOPLE + 3] = 8.12;
  graph[7 * NUM_PEOPLE + 4] = 271.38;

  printCsv(graph);
  printf("\n");
  simplify(graph, NUM_PEOPLE);
  printGraph(graph, NUM_PEOPLE);
  printf("\n");
  simplify(graph, NUM_PEOPLE);
  printCsv(graph);
}
