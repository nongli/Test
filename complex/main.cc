#include <stdio.h>
#include <string.h>
#include <vector>

using namespace std;

struct Records {
  // One column for each leaf node + each internal list node.
  // This is an pre order traversal of the schema (so all the leaf nodes
  // are at the end).
  vector<vector<int> > columns;
};

// Schema is list<list<int>>
// Data is:
//  [
//    [0, 1]
//  ],
//  [
//    [2, 3], [4, 5, 6]
//  ]
//  [
//    [7], [8], [9]
//  ]
void Example1() {
  Records records;
  int values[] =     { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  int rep_levels[] = { 0, 2, 0, 2, 1, 2, 2, 0, 1, 1 };

  int num_values = sizeof(values) / sizeof(int);
  printf("%d\n", num_values);

  // We know from the schema there are two lists and one leaf node.
  const int NUM_INTERNAL_NODES = 2;
  const int SCHEMA_SIZE = NUM_INTERNAL_NODES + 1;
  records.columns.resize(SCHEMA_SIZE);

  // The leaf columns just point to the underlying data.
  // This doesn't have to be a copy but could just be a ptr to values.
  records.columns[2].resize(num_values);
  memcpy(records.columns[2].data(), values, sizeof(values));

  int values_at_node[SCHEMA_SIZE];
  for (int i = 0; i < SCHEMA_SIZE; ++i) {
    values_at_node[i] = 0;
  }

  // For each (value, rep_level) pair, construct the internal node values.
  // Each internal node is a list and the value is the number of children it has.
  for (int i = 0; i < num_values; ++i) {
    int rep_level = rep_levels[i];
    if (rep_level < NUM_INTERNAL_NODES) {
      // For non-leaf rep_levels, we're starting a new list. Push the old
      // list size to columns[rep_level]. (Unless this is the first time,
      // which is the only time values_at_node is 0).
      // We need to "flush" at all the levels lower than this (i.e. the subtree
      // of the schema at this node).
      for (int l = rep_level; l < NUM_INTERNAL_NODES; ++l) {
        if (values_at_node[l] != 0) {
          records.columns[l].push_back(values_at_node[l]);
        }
        values_at_node[l] = 1;
      }
    }
    if (rep_level != 0) {
      ++values_at_node[rep_level - 1];
    }
  }
  // Finish off the last buffered record
  if (num_values > 0) {
    for (int i = 0; i < NUM_INTERNAL_NODES; ++i) {
      records.columns[i].push_back(values_at_node[i]);
    }
  }

  // Outputting results.

  printf("Num assembled records: %ld\n", records.columns[0].size());
  for (int i = 0; i < SCHEMA_SIZE; ++i) {
    for (int j = 0; j < records.columns[i].size(); ++j) {
      printf("%d ", records.columns[i][j]);
    }
    printf("\n");
  }

  printf("Assembled records:\n");
  int outer_list_idx = 0;  // Index into columns[0]
  int inner_list_idx = 0;  // Index into columns[1]
  int data_idx = 0;        // Index into columns[2]

  for (; outer_list_idx < records.columns[0].size(); ++outer_list_idx) {
    int num_in_outer_list = records.columns[0][outer_list_idx];
    printf("[\n");
    for (int j = 0; j < num_in_outer_list; ++j, ++inner_list_idx) {
      int num_in_inner_list = records.columns[1][inner_list_idx];
      if (j == 0) {
        printf("  [");
      } else {
        printf(",[");
      }
      for (int k = 0; k < num_in_inner_list; ++k, ++data_idx) {
        if (k != 0) {
          printf(",%d", records.columns[2][data_idx]);
        } else {
          printf("%d", records.columns[2][data_idx]);
        }
      }
      printf("]");
    }
    printf("\n],\n");
  }
}

int main(int argc, char** argv) {

  Example1();

  printf("Done.\n");
  return 0;
}
