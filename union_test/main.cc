#include <iostream>
#include <stdlib.h>

using namespace std;

unsigned long Rdtsc() {
  unsigned int lo, hi;
  __asm__ __volatile__ (      
    "xorl %%eax,%%eax \n        cpuid"
    ::: "%rax", "%rbx", "%rcx", "%rdx");
  __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
  return (unsigned long)hi << 32 | lo;
}

struct Foo {
  union {
    bool bool_val;
    char tinyint_val;
    short smallint_val;
    int int_val;
    long long_val;
    float float_val;
    double double_val;
    char data[16];
  };
  bool flag;
};

Foo __attribute__ ((noinline)) OpInt(const Foo& input, int i) {
  Foo f;
  f.int_val = input.int_val + i;
  return f;
}

Foo __attribute__ ((noinline)) OpInt2(const Foo& input, int i) {
  Foo f;
  int val = *reinterpret_cast<const int*>(&input.data[0]);
  *reinterpret_cast<long*>(&f.data[0]) = val + i;
  return f;
}

Foo __attribute__ ((noinline)) OpLong(const Foo& input, long i) {
  Foo f;
  f.long_val = input.long_val + i;
  return f;
}

long ITERATIONS = 1024 * 1024;

int main(int argc, char** argv) {
  int test_case = 1;
  if (argc == 2) {
    test_case = atoi(argv[1]);
  }

  Foo f;

  if (test_case & 1) {
    unsigned long int_start = Rdtsc();
    for (int j = 0; j < ITERATIONS; ++j) {
      f.int_val = j;
      for (int i = 1; i < 1000; ++i) {
        f = OpInt(f, i);
      }
    }
    cout << "Result: " << f.int_val << endl;
    unsigned long int_cycles = Rdtsc() - int_start;
    cout << "Int cycles:\t" << int_cycles << endl;
  }
  
  if (test_case & 2) {
    unsigned long long_start = Rdtsc();
    for (int j = 0; j < ITERATIONS; ++j) {
      f.long_val = j;
      for (int i = 1; i < 1000; ++i) {
        f = OpLong(f, i);
      }
    }
    cout << "Result: " << f.long_val << endl;
    unsigned long long_cycles = Rdtsc() - long_start;
    cout << "Long cycles:\t" << long_cycles << endl;
  }

  if (test_case & 4) {
    unsigned long int2_start = Rdtsc();
    for (int j = 0; j < ITERATIONS; ++j) {
      *reinterpret_cast<int*>(&f.data[0]) = j;
      for (int i = 1; i < 1000; ++i) {
        f = OpInt2(f, i);
      }
    }
    cout << "Result: " << *reinterpret_cast<int*>(&f.data[0]) << endl;
    unsigned long int2_cycles = Rdtsc() - int2_start;
    cout << "Int2 cycles:\t" << int2_cycles << endl;
  }

  return f.int_val;
}

