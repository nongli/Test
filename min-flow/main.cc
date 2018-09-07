#include <stdio.h>
#include <string.h>

#include <map>
#include <string>
#include <vector>
using namespace std;

bool simplify(float* g, int N, int srcRow, int srcCol, int dstRow, int dstCol) {
  float v1 = g[srcCol * N + dstCol];
  float v2 = g[dstCol * N + srcCol];
  if (v1 != 0) {
    if (g[srcRow * N + srcCol] < v1 && g[srcRow * N + dstCol] != 0) {
      g[srcCol * N + dstCol] -= g[srcRow * N + srcCol];
      g[srcRow * N + dstCol] += g[srcRow * N + srcCol];
      g[srcRow * N + srcCol] = 0;
      return true;
    }
  } else if (v2 != 0) {
    if (g[dstRow * N + dstCol] < v2 && g[srcRow * N + srcCol] != 0) {
      g[dstCol * N + srcCol] -= g[dstRow * N + dstCol];
      g[srcRow * N + srcCol] += g[dstRow * N + dstCol];
      g[dstRow * N + dstCol] = 0;
      return true;
    }
    if (g[dstRow * N + dstCol] > v2 && g[srcRow * N + srcCol] != 0) {
      g[dstRow * N + srcCol] += v2;
      g[dstRow * N + dstCol] -= v2;
      g[dstCol * N + srcCol] = 0;
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

class Graph {
 public:
  Graph(const std::vector<std::string>& people) {
    people_ = people;
    for (int i = 0; i < people.size(); ++i) {
      people_idx_[people[i]] = i;
    }
    graph_ = new float[people.size() * people.size()];
    memset(graph_, 0, sizeof(float) * people.size() * people.size());
  }

  /**
   * Add an expense just for these participants
   */
  void AddExpense(const std::string& payer, float amt,
      const std::vector<std::string>& participants) {
    for (int i = 0; i < participants.size(); i++) {
      int part_id = people_idx_[participants[i]];
      graph_[part_id * people_.size() + people_idx_[payer]] += amt / participants.size();
    }
  }

  /**
   * Add an expense just for all participants
   */
  void AddExpense(const std::string& payer, float amt) {
    for (int i = 0; i < people_.size(); i++) {
      graph_[i * people_.size() + people_idx_[payer]] += amt / people_.size();
    }
  }

  /**
   * Prints the payout as a csv table
   */
  void PrintCsv() {
    int N = people_.size();
    printf("            ");
    for (int i = 0; i < N; i++) {
      printf("%-10s", people_[i].c_str());
    }
    printf("\n");
    const char* sep = "%-10s";
    //const char* sep = "\t%s";
    for (int i = 0; i < N; i++) {
      printf("%-10s  ", people_[i].c_str());
      for (int j = 0; j < N; j++) {
        if (graph_[i * N + j] == 0) {
          printf(sep, "\\");
        } else {
          char buf[256];
          sprintf(buf, "%0.2f", graph_[i * N + j]);
          printf(sep, buf);
        }
      }
      printf("\n");
    }
  }

  void Simplify() {
    SimplifyTrivial();
    simplify(graph_, people_.size());
    simplify(graph_, people_.size());
    SimplifySinks();
  }

  void PrintGraph() {
    for (int i = 0; i < people_.size(); i++) {
      for (int j = 0; j < people_.size(); j++) {
        printf("%6.2f ", graph_[i * people_.size() + j]);
      }
      printf("\n");
    }
  }

 private:
  std::vector<std::string> people_;
  std::map<std::string, int> people_idx_;
  float* graph_;

  void SimplifyTrivial() {
    // Clear out owning oneself
    for (int i = 0; i < people_.size(); ++i) {
      graph_[i * people_.size() + i] = 0;
    }

    // Clear out where two people own each other
    for (int i = 0; i < people_.size(); i++) {
      for (int c = i + 1; c < people_.size(); c++) {
        if (graph_[i * people_.size() + c] == 0) continue;
        if (graph_[c * people_.size() + i] == 0) continue;

        if (graph_[i * people_.size() + c] > graph_[c * people_.size() + i]) {
          graph_[i * people_.size() + c] -= graph_[c * people_.size() + i];
          graph_[c * people_.size() + i] = 0;
        } else {
          graph_[c * people_.size() + i] -= graph_[i * people_.size() + c];
          graph_[i * people_.size() + c] = 0;
        }
      }
    }
  }

  float v(int row, int col) {
    return graph_[row * people_.size() + col];
  }

  void set(int row, int col, float v) {
    graph_[row * people_.size() + col] = v;
  }

  /**
   * Look for the case where A and B both owe C and D. One of A-C,A-D,B-C or B-D
   * can be deleted.
   */
  void SimplifySinks() {
    bool changed = true;
    while (changed) {
      changed = false;
      for (int c = 0; c < people_.size(); c++) {
        for (int d = c + 1; d < people_.size(); d++) {
          int a = -1;
          for (int i = 0; i < people_.size(); i++) {
            if (v(i, c) != 0 && v(i, d) != 0) {
              // Found someone that owes both c and d
              if (a == -1) {
                a = i;
              } else {
                int b = i;
                float ac = v(a, c);
                float ad = v(a, d);
                float bc = v(b, c);
                float bd = v(b, d);
                if (bd < bc && bd < ad && bd < ac) {
                  // bd is smallest
                  bc += bd;
                  ac -= bd;
                  ad += bd;
                  bd -= bd;
                  changed = true;
                } else if (ac < ad && ac < bc && ac < bd) {
                  // ac is smallest
                  bc += ac;
                  bd -= ac;
                  ad += ac;
                  ac -= ac;
                  changed = true;
                } else if (ad < ac && ad < bc && ad < bd) {
                  // ad is smallest
                  bd += ad;
                  bc -= ad;
                  ac += ad;
                  ad -= ad;
                  changed = true;
                } else {
                  printf("Found someone else.\n");
                  printf("%f %f %f %f\n", ac, ad, bc, bd);
                }

                set(a, c, ac);
                set(a, d, ad);
                set(b, c, bc);
                set(b, d, bd);
              }
            }
          }
        }
      }
    }
  }
};

int main(int argc, char** argv) {
  Graph g({
    "Marshall",
    "Mark",
    "Itay",
    "Nong",
    "Panda",
    "Greg",
    "Joe"}
  );

  // 2
  g.AddExpense("Itay", 2034.83);
  // 3
  g.AddExpense("Marshall", 38, {"Itay", "Marshall"});
  // 4
  g.AddExpense("Itay", 16.55, {"Itay", "Marshall"});
  // 6
  g.AddExpense("Itay", 730, {"Itay", "Marshall", "Greg", "Nong", "Panda"});
  // 8
  g.AddExpense("Itay", 82.08, {"Itay", "Marshall", "Nong", "Panda", "Joe"});
  // 9
  g.AddExpense("Itay", 745.39, {"Itay", "Marshall", "Greg", "Nong", "Panda", "Joe"});
  // 10
  g.AddExpense("Panda", 100, {"Itay", "Marshall", "Greg", "Nong", "Panda", "Joe"});
  // 11
  g.AddExpense("Marshall", 100);
  // 12
  g.AddExpense("Nong", 910);
  // 13
  g.AddExpense("Marshall", 497);
  // 14
  g.AddExpense("Mark", 65, {"Nong", "Marshall", "Mark", "Joe", "Itay"});
  // 15
  g.AddExpense("Nong", 94, {"Nong", "Marshall", "Mark", "Joe", "Itay"});
  // 16
  g.AddExpense("Greg", 87, {"Greg", "Marshall", "Panda"});
  // 17
  g.AddExpense("Greg", 92);
  // 18
  g.AddExpense("Panda", 63, {"Nong", "Marshall", "Panda", "Itay", "Greg"});
  // 22
  g.AddExpense("Nong", 1550, {"Nong", "Marshall", "Panda", "Itay", "Greg"});

  g.PrintCsv();
  printf("\n");
  g.Simplify();
  g.PrintCsv();
}
