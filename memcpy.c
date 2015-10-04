#include <stdio.h>
#include <string.h>
#include <boost/cstdint.hpp>

uint64_t Rdtsc() {
  uint32_t lo, hi;
  __asm__ __volatile__ (      
    "xorl %%eax,%%eax \n        cpuid"
    ::: "%rax", "%rbx", "%rcx", "%rdx");
  __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
  return (uint64_t)hi << 32 | lo;
}

int main() {
  uint64_t buffer_size = 100 * 1024 * 1024;
  double total_size = 100 * 1024 * 1024 * 1024.0;
  char* src = (char*)malloc(buffer_size);
  char* dst = (char*)malloc(buffer_size);

  uint64_t start = Rdtsc();
  uint64_t iterations = (int)(total_size / buffer_size);
  for (uint64_t n = 0; n < iterations; ++n) {
    memcpy(dst, src, buffer_size);
  }
  uint64_t end = Rdtsc();
  
  double cycles_per_second = 3392.48 * 1000 * 1000;  // hard-coded to my desktop
  double seconds = (end - start ) / cycles_per_second;
  printf("Total Size: %f Batch Size: %ld Iterations: %ld\n", total_size, buffer_size, iterations);
  printf("Time %f sec\n", seconds);
  printf("Rate: %f GB/s\n", total_size / (1024 * 1024 * 1024) / seconds);
}
