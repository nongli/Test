#include <list>
#include <vector>
#include <iostream>
#include <boost/cstdint.hpp>

using namespace std;

#define N 100000

uint64_t Rdtsc() {
  uint32_t lo, hi;
  __asm__ __volatile__ (      
    "xorl %%eax,%%eax \n        cpuid"
    ::: "%rax", "%rbx", "%rcx", "%rdx");
  __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
  return (uint64_t)hi << 32 | lo;
}

struct Pair {
  int x, y;

  Pair(int v) {
    x = y = v;
  }

  uint64_t operator+(uint64_t v) const {
    return v + x;
  }
  
  uint64_t operator*() const {
    return x;
  }

  Pair& operator=(int v) {
    x = y = v;
  }
};

#define CONTAINER vector
template <typename T>
void Test() {
  CONTAINER<T> v;

  for (int i = 0; i < N; ++i) {
    v.push_back(rand() % N);
  }

  uint64_t sum = 0;
  uint64_t start = 0;

  /*
  // C-style array
  sum = 0;
  start = Rdtsc();
  for (int i = 0; i < N; ++i) {
    for (int j = i; j < N; ++j) {
      sum = v[j] + sum;
    }
  }
  cout << sum << endl;
  cout << "C-style Cycles " << (Rdtsc() - start) / (1024 * 1024 * 1024.0) << endl;
  */
  
  // iterator post increment
  sum = 0;
  start = Rdtsc();
  typename CONTAINER<T>::const_iterator j = v.begin();
  for (int i = 0; i < N; ++i) {
    for (typename CONTAINER<T>::const_iterator it = j; it != v.end(); it++) {
      sum = *it + sum;
    }
    ++j;
  }
  cout << sum << endl;
  cout << "Iter-Post Cycles " << (Rdtsc() - start) / (1024 * 1024 * 1024.0) << endl;
  
  // iterator calling end() each time
  sum = 0;
  start = Rdtsc();
  j = v.begin();
  for (int i = 0; i < N; ++i) {
    for (typename CONTAINER<T>::const_iterator it = j; it != v.end(); ++it) {
      sum = *it + sum;
    }
    ++j;
  }
  cout << sum << endl;
  cout << "Iter-Pre Cycles " << (Rdtsc() - start) / (1024 * 1024 * 1024.0) << endl;
  
  // iterator caching end()
  sum = 0;
  start = Rdtsc();
  j = v.begin();
  for (int i = 0; i < N; ++i) {
    typename CONTAINER<T>::const_iterator e = v.end();
    for (typename CONTAINER<T>::const_iterator it = j; it != e; ++it) {
      sum = *it + sum;
    }
    ++j;
  }
  cout << sum << endl;
  cout << "Iter-End Cycles " << (Rdtsc() - start) / (1024 * 1024 * 1024.0) << endl;
}

int main(int argc, char** argv) {
  Test<int>();
  Test<Pair>();
}
