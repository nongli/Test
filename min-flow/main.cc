#include <stdio.h>
#include <string.h>

#include <vector>
using namespace std;

const char* PEOPLE[] = {
  "Marshall",
  "Mark",
  "Itay",
  "Nong",
  "Panda",
  "Greg",
  "Joe",
};

const int NUM_PEOPLE = sizeof(PEOPLE) / sizeof(PEOPLE[0]);

void printCsv(float* g) {
  printf("            ");
  for (int i = 0; i < NUM_PEOPLE; i++) {
    printf("%-10s", PEOPLE[i]);
  }
  printf("\n");
  //const char* sep = "%-10s";
  const char* sep = "\t%s";
  for (int i = 0; i < NUM_PEOPLE; i++) {
    printf("%-10s  ", PEOPLE[i]);
    for (int j = 0; j < NUM_PEOPLE; j++) {
      if (g[i * NUM_PEOPLE + j] == 0) {
        printf(sep, "\\");
      } else {
        char buf[256];
        sprintf(buf, "%0.2f", g[i * NUM_PEOPLE + j]);
        printf(sep, buf);
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
    if (g[dstRow * NUM_PEOPLE + dstCol] > v2 && g[srcRow * NUM_PEOPLE + srcCol] != 0) {
      g[dstRow * NUM_PEOPLE + srcCol] += v2;
      g[dstRow * NUM_PEOPLE + dstCol] -= v2;
      g[dstCol * NUM_PEOPLE + srcCol] = 0;
      return true;
    }
  }

  return false;
}

bool simplify(float* g, int w) {
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

  for (int i = w - 1; i >= 0; i--) {
    for (int c = w - 1; c >= 0; c--) {
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

void simplifyTrivial(float* g, int w) {
  for (int i = 0; i < w; i++) {
    for (int c = i + 1; c < w; c++) {
      if (g[i * w + c] == 0) continue;
      if (g[c * w + i] == 0) continue;

      if (g[i * w + c] > g[c * w + i]) {
        g[i * w + c] -= g[c * w + i];
        g[c * w + i] = 0;
      } else {
        g[c * w + i] -= g[i * w + c];
        g[i * w + c] = 0;
      }
    }
  }
}

int main(int argc, char** argv) {
  float* graph = new float[NUM_PEOPLE * NUM_PEOPLE];
  memset(graph, 0, sizeof(float) * NUM_PEOPLE * NUM_PEOPLE);

  // 2
  graph[0 * NUM_PEOPLE + 2] += 290.69;
  graph[1 * NUM_PEOPLE + 2] += 290.69;
  graph[2 * NUM_PEOPLE + 2] += 290.69;
  graph[3 * NUM_PEOPLE + 2] += 290.69;
  graph[4 * NUM_PEOPLE + 2] += 290.69;
  graph[5 * NUM_PEOPLE + 2] += 290.69;
  graph[6 * NUM_PEOPLE + 2] += 290.69;

  // 3
  graph[0 * NUM_PEOPLE + 0] += 16;
  graph[2 * NUM_PEOPLE + 0] += 16;

  // 4
  graph[0 * NUM_PEOPLE + 2] += 8.55;
  graph[2 * NUM_PEOPLE + 2] += 8;

  // 6
  graph[0 * NUM_PEOPLE + 2] += 146;
  graph[2 * NUM_PEOPLE + 2] += 146;
  graph[3 * NUM_PEOPLE + 2] += 146;
  graph[4 * NUM_PEOPLE + 2] += 146;
  graph[5 * NUM_PEOPLE + 2] += 146;

  // 8
  graph[0 * NUM_PEOPLE + 2] += 16.4;
  graph[2 * NUM_PEOPLE + 2] += 16.4;
  graph[3 * NUM_PEOPLE + 2] += 16.4;
  graph[4 * NUM_PEOPLE + 2] += 16.4;
  graph[6 * NUM_PEOPLE + 2] += 16.4;

  // 9
  graph[0 * NUM_PEOPLE + 2] += 124.23;
  graph[2 * NUM_PEOPLE + 2] += 124.23;
  graph[3 * NUM_PEOPLE + 2] += 124.23;
  graph[4 * NUM_PEOPLE + 2] += 124.23;
  graph[5 * NUM_PEOPLE + 2] += 124.23;
  graph[6 * NUM_PEOPLE + 2] += 124.23;

  // 10
  graph[0 * NUM_PEOPLE + 4] += 16.66;
  graph[2 * NUM_PEOPLE + 4] += 16.66;
  graph[3 * NUM_PEOPLE + 4] += 16.66;
  graph[4 * NUM_PEOPLE + 4] += 16.66;
  graph[5 * NUM_PEOPLE + 4] += 16.66;
  graph[6 * NUM_PEOPLE + 4] += 16.66;

  // 11
  graph[0 * NUM_PEOPLE + 0] += 14;
  graph[1 * NUM_PEOPLE + 0] += 14;
  graph[2 * NUM_PEOPLE + 0] += 14;
  graph[3 * NUM_PEOPLE + 0] += 14;
  graph[4 * NUM_PEOPLE + 0] += 14;
  graph[5 * NUM_PEOPLE + 0] += 14;
  graph[6 * NUM_PEOPLE + 0] += 14;

  // 12
  graph[0 * NUM_PEOPLE + 3] += 130;
  graph[1 * NUM_PEOPLE + 3] += 130;
  graph[2 * NUM_PEOPLE + 3] += 130;
  graph[3 * NUM_PEOPLE + 3] += 130;
  graph[4 * NUM_PEOPLE + 3] += 130;
  graph[5 * NUM_PEOPLE + 3] += 130;
  graph[6 * NUM_PEOPLE + 3] += 130;

  // 13
  graph[0 * NUM_PEOPLE + 0] += 71;
  graph[1 * NUM_PEOPLE + 0] += 71;
  graph[2 * NUM_PEOPLE + 0] += 71;
  graph[3 * NUM_PEOPLE + 0] += 71;
  graph[4 * NUM_PEOPLE + 0] += 71;
  graph[5 * NUM_PEOPLE + 0] += 71;
  graph[6 * NUM_PEOPLE + 0] += 71;

  // 15
  graph[0 * NUM_PEOPLE + 3] += 18.8;
  graph[1 * NUM_PEOPLE + 3] += 18.8;
  graph[2 * NUM_PEOPLE + 3] += 18.8;
  graph[3 * NUM_PEOPLE + 3] += 18.8;
  graph[6 * NUM_PEOPLE + 3] += 18.8;

  // 16
  graph[0 * NUM_PEOPLE + 5] += 29;
  graph[4 * NUM_PEOPLE + 5] += 29;
  graph[5 * NUM_PEOPLE + 5] += 29;

  // 17
  graph[0 * NUM_PEOPLE + 5] += 13.14;
  graph[1 * NUM_PEOPLE + 5] += 13.14;
  graph[2 * NUM_PEOPLE + 5] += 13.14;
  graph[3 * NUM_PEOPLE + 5] += 13.14;
  graph[4 * NUM_PEOPLE + 5] += 13.14;
  graph[5 * NUM_PEOPLE + 5] += 13.14;
  graph[6 * NUM_PEOPLE + 5] += 13.14;

  // 18
  graph[0 * NUM_PEOPLE + 4] += 12.6;
  graph[2 * NUM_PEOPLE + 4] += 12.6;
  graph[3 * NUM_PEOPLE + 4] += 12.6;
  graph[4 * NUM_PEOPLE + 4] += 12.6;
  graph[5 * NUM_PEOPLE + 4] += 12.6;

  // 22
  graph[0 * NUM_PEOPLE + 3] += 310;
  graph[2 * NUM_PEOPLE + 3] += 310;
  graph[3 * NUM_PEOPLE + 3] += 310;
  graph[4 * NUM_PEOPLE + 3] += 310;
  graph[5 * NUM_PEOPLE + 3] += 310;

  printCsv(graph);
  printf("\n");

  // Clear out owning oneself
  for (int i = 0; i < NUM_PEOPLE; ++i) {
    graph[i * NUM_PEOPLE + i] = 0;
  }

  simplifyTrivial(graph, NUM_PEOPLE);
  simplify(graph, NUM_PEOPLE);
  printGraph(graph, NUM_PEOPLE);
  printf("\n");
  simplify(graph, NUM_PEOPLE);

  printCsv(graph);
}
