#include <stdio.h>
#include <string.h>

#include <vector>
using namespace std;

void printCsv(float* g) {
  for (int i = 0; i < 8; i++) {
    switch (i) {
      case 0:
        printf("Marshall");
        break;
      case 1:
        printf("Ariana");
        break;
      case 2:
        printf("Itay");
        break;
      case 3:
        printf("Nong");
        break;
      case 4:
        printf("Panda");
        break;
      case 5:
        printf("Mike");
        break;
      case 6:
        printf("Nisha");
        break;
      case 7:
        printf("Skye");
        break;
    }
    for (int j = 0; j < 8; j++) {
      if (g[i * 8 + j] == 0) {
        printf("\t/");
      } else {
        printf("\t%0.2f", g[i * 8 + j]);
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
  //printf("Trying: %d,%d %d,%d\n", srcRow, srcCol, dstRow, dstCol);
  float v1 = g[srcCol * 8 + dstCol];
  float v2 = g[dstCol * 8 + srcCol];
  if (v1 != 0) {
    printf("1 %f\n", v1);
  } else if (v2 != 0) {
    if (g[dstRow * 8 + dstCol] < v2) {
      printf("Removing %d %d\n", dstRow, dstCol);
      g[dstCol * 8 + srcCol] -= g[dstRow * 8 + dstCol];
      g[srcRow * 8 + srcCol] += g[dstRow * 8 + dstCol];
      g[dstRow * 8 + dstCol] = 0;
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
  float* graph = new float[8 * 8];
  memset(graph, 0, sizeof(float) * 8 * 8);
  graph[0 * 8 + 2] = 381.81;
  graph[0 * 8 + 4] = 83.46;

  graph[1 * 8 + 0] = 231.75;
  graph[1 * 8 + 2] = 467.09;
  graph[1 * 8 + 3] = 30.29;
  graph[1 * 8 + 4] = 315.21;
  graph[1 * 8 + 5] = 6.61;
  graph[1 * 8 + 7] = 36.26;

  graph[3 * 8 + 0] = 198.96;
  graph[3 * 8 + 2] = 467.49;
  graph[3 * 8 + 4] = 282.43;

  graph[4 * 8 + 2] = 0;

  graph[5 * 8 + 0] = 235.47;
  graph[5 * 8 + 2] = 460.49;
  graph[5 * 8 + 3] = 23.68;
  graph[5 * 8 + 4] = 283.08;
  graph[5 * 8 + 7] = 23.43;

  graph[6 * 8 + 0] = 75.61;
  graph[6 * 8 + 2] = 467.09;
  graph[6 * 8 + 3] = 30.29;
  graph[6 * 8 + 4] = 203.92;
  graph[6 * 8 + 5] = 6.61;
  graph[6 * 8 + 7] = 30.04;

  graph[7 * 8 + 0] = 207.08;
  graph[7 * 8 + 2] = 422.43;
  graph[7 * 8 + 3] = 8.12;
  graph[7 * 8 + 4] = 271.38;

  printCsv(graph);
  simplify(graph, 8);
  printGraph(graph, 8);
  printCsv(graph);
}
