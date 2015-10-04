#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <vector>
#include <boost/cstdint.hpp>
#include <iostream>

#define MAX_CHILDREN 15
 
uint64_t Rdtsc() {
  uint32_t lo, hi;
  __asm__ __volatile__ (      
    "xorl %%eax,%%eax \n        cpuid"
    ::: "%rax", "%rbx", "%rcx", "%rdx");
  __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
  return (uint64_t)hi << 32 | lo;
}

struct Node {
  int num_values;
  int values[MAX_CHILDREN - 1];
  Node* children[MAX_CHILDREN];

  Node() {
    num_values = 0;
    memset(children, 0, sizeof(Node*) * MAX_CHILDREN);
  }
};

void Insert(Node* node, int val);

void InsertAt(Node* parent, int index, int val) {
  if (parent->children[index] == NULL) {
    parent->children[index] = new Node;
  }

  Insert(parent->children[index], val);
}

void Insert(Node* node, int val) {
  if (node->num_values < MAX_CHILDREN - 1) {
    // Insert val into node->values
    for (int i = 0; i < node->num_values; ++i) {
      // Ignore duplicates
      if (val == node->values[i]) return;
      if (val < node->values[i]) {
        int values_to_move = node->num_values - i;
        if (values_to_move != 0) {
          memmove(&node->values[i+1], &node->values[i], sizeof(int) * values_to_move);
        }
        node->values[i] = val;
        ++node->num_values;
        return;
      }
    }
    node->values[node->num_values++] = val;
    return;
  }

  for (int i = 0; i < MAX_CHILDREN - 1; ++i) {
    if (val < node->values[i]) {
      return InsertAt(node, i, val);
    }
  }
  return InsertAt(node, MAX_CHILDREN - 1, val);
}

void Print(Node* node, int indent) {
  for (int i = 0; i < indent; ++i) {
    printf(" ");
  }
  for (int i = 0; i < node->num_values; ++i) {
    printf("%d ", node->values[i]);
  }
  printf("\n");
  for (int i = 0; i < MAX_CHILDREN; ++i) {
    if (node->children[i] != NULL) Print(node->children[i], indent + 1);
  }
}

bool Find(Node* node, int val, double* crap) {
  int min = INT_MIN;
  
  while (node != NULL) {
    Node* next_node = NULL;
    for (int i = 0; i < node->num_values; ++i) {
      if (node->values[i] == val) return true;
      if (val > min && val < node->values[i]) {
        next_node = node->children[i];
        break;
      }
      min = node->values[i];
    } 
    if (next_node == NULL) next_node = node->children[node->num_values];
    
    for (int i = 0; i < sizeof(Node)/64; i++) { 
      __builtin_prefetch((char*)next_node + i*64); 
    }
    //__builtin_prefetch(next_node);

    // Do some work
    *crap = sin(*crap);
    //*crap = sin(*crap);

    node = next_node;
  }

  return false;
}

int LeftMostValue(Node* node) {
  if (node->children[0] != NULL) return LeftMostValue(node->children[0]);
  return node->values[0];
}

int main(int argc, char** argv) {
  Node root;

  printf("Node size: %d\n", sizeof(Node));

  int num_items = 3000000;

  for (int i = 0; i < num_items; ++i) {
    int r = rand() % (num_items * 10);
    Insert(&root, r);
  }

  int left_most = LeftMostValue(&root);
  printf("Leftmost value %d\n", left_most);

  uint64_t start = Rdtsc();
  int num_found = 0;
  double dummy = M_PI;
  for (int i = 0; i < num_items * 10; ++i) {
    num_found += Find(&root, i, &dummy);
//    num_found += Find(&root, left_most, &dummy);
  }
  uint64_t duration = Rdtsc() - start;

  printf("%d out of %d\n", num_found, num_items);
  std::cout << "Cycles (billions): " << duration / (1024 * 1024 * 1024.0) << std::endl;
  return dummy;
}
