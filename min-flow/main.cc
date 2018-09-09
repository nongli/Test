#include <stdio.h>
#include <string.h>

#include <map>
#include <string>
#include <vector>
using namespace std;

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
      add(part_id, people_idx_[payer], amt / participants.size());
    }
  }

  /**
   * Add an expense just for all participants
   */
  void AddExpense(const std::string& payer, float amt) {
    for (int i = 0; i < people_.size(); i++) {
      add(i, people_idx_[payer], amt / people_.size());
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
        if (get(i, j) == 0) {
          printf(sep, "\\");
        } else {
          char buf[256];
          sprintf(buf, "%0.2f", get(i, j));
          printf(sep, buf);
        }
      }
      printf("\n");
    }
  }

  void Simplify() {
    SimplifyTrivial();
    bool changed;
    do {
      changed = SimplifyInternal();
    } while (changed);
    SimplifySinks();
  }

  void PrintGraph() {
    for (int i = 0; i < people_.size(); i++) {
      for (int j = 0; j < people_.size(); j++) {
        printf("%6.2f ", get(i, j));
      }
      printf("\n");
    }
  }

 private:
  std::vector<std::string> people_;
  std::map<std::string, int> people_idx_;
  float* graph_;

  float get(int row, int col) const {
    return graph_[row * people_.size() + col];
  }

  void set(int row, int col, float v) {
    graph_[row * people_.size() + col] = v;
  }

  void add(int row, int col, float v) {
    graph_[row * people_.size() + col] += v;
  }

  void SimplifyTrivial() {
    // Clear out owning oneself
    for (int i = 0; i < people_.size(); ++i) {
      set(i, i, 0);
    }

    // Clear out where two people own each other
    for (int i = 0; i < people_.size(); i++) {
      for (int c = i + 1; c < people_.size(); c++) {
        if (get(i, c) == 0) continue;
        if (get(c, i) == 0) continue;

        if (get(i, c) > get(c, i)) {
          add(i, c, -get(c, i));
          set(c, i, 0);
        } else {
          add(c, i, -get(i, c));
          set(i, c, 0);
        }
      }
    }
  }

  bool SimplifyInternal() {
    bool changed = false;
    for (int i = 0; i < people_.size(); i++) {
      for (int c = 0; c < people_.size(); c++) {
        if (get(i, c) == 0) continue;

        for (int owes = 0; owes < people_.size(); owes++) {
          if (owes != c && get(i, owes) != 0) {
            changed |= Simplify(i, c, i, owes);
          }
        }
      }
    }
    return changed;
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
            if (get(i, c) != 0 && get(i, d) != 0) {
              // Found someone that owes both c and d
              if (a == -1) {
                a = i;
              } else {
                int b = i;
                float ac = get(a, c);
                float ad = get(a, d);
                float bc = get(b, c);
                float bd = get(b, d);
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

  bool Simplify(int srcRow, int srcCol, int dstRow, int dstCol) {
    float v1 = get(srcCol, dstCol);
    float v2 = get(dstCol, srcCol);
    if (v1 != 0) {
      if (get(srcRow, srcCol) < v1 && get(srcRow, dstCol) != 0) {
        add(srcCol, dstCol, -get(srcRow, srcCol));
        add(srcRow, dstCol, get(srcRow, srcCol));
        set(srcRow, srcCol, 0);
        return true;
      }
      if (get(srcRow, srcCol) > v1 && get(srcRow, dstCol) != 0) {
        printf("Should do something.\n");
      }
    } else if (v2 != 0) {
      if (get(dstRow, dstCol) < v2 && get(srcRow, srcCol) != 0) {
        add(dstCol, srcCol, -get(dstRow, dstCol));
        add(srcRow, srcCol, get(dstRow, dstCol));
        set(dstRow, dstCol, 0);
        return true;
      }
      if (get(dstRow, dstCol) > v2 && get(srcRow, srcCol) != 0) {
        add(dstRow, srcCol, v2);
        add(dstRow, dstCol, -v2);
        set(dstCol, srcCol, 0);
        return true;
      }
    }

    return false;
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
