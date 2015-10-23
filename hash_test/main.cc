#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <time.h>

#include <boost/cstdint.hpp>
#include <benchmark/benchmark.h>

#include "xxhash.cc"
#include "MurmurHash3.h"

using namespace std;

static const uint64_t MURMUR_PRIME = 0xc6a4a7935bd1e995;
static const int MURMUR_R = 47;

enum HASH_FN {
  MURMUR2,
  XXHASH,
  XXHASH_NONG,
  MURMUR3,
};

const HASH_FN FN = MURMUR3;

// Murmur2 hash implementation returning 64-bit hashes.
static uint64_t MurmurHash2_64(const void* input, int len, uint64_t seed) {
  uint64_t h = seed ^ (len * MURMUR_PRIME);

  const uint64_t* data = reinterpret_cast<const uint64_t*>(input);
  const uint64_t* end = data + (len / sizeof(uint64_t));

  while (data != end) {
    uint64_t k = *data++;
    k *= MURMUR_PRIME;
    k ^= k >> MURMUR_R;
    k *= MURMUR_PRIME;
    h ^= k;
    h *= MURMUR_PRIME;
  }

  const uint8_t* data2 = reinterpret_cast<const uint8_t*>(data);
  switch (len & 7) {
    case 7: h ^= uint64_t(data2[6]) << 48;
    case 6: h ^= uint64_t(data2[5]) << 40;
    case 5: h ^= uint64_t(data2[4]) << 32;
    case 4: h ^= uint64_t(data2[3]) << 24;
    case 3: h ^= uint64_t(data2[2]) << 16;
    case 2: h ^= uint64_t(data2[1]) << 8;
    case 1: h ^= uint64_t(data2[0]);
            h *= MURMUR_PRIME;
  }

  h ^= h >> MURMUR_R;
  h *= MURMUR_PRIME;
  h ^= h >> MURMUR_R;
  return h;
}

const int N = 64;
const int STRIDE = 2;

#define PREAMBLE\
  vector<int64_t> vals;                       \
  vals.resize(N);                             \
  for (int i = 0; i < N; ++i) {               \
    vals[i] = rand();                         \
  }                                           \
  uint64_t h = 0;                             \
  uint64_t iterations = 0;                    \
  while (state.KeepRunning()) {               \
    ++iterations;                             \
    for (int i = 0; i < N; i += STRIDE)

#define EPILOG \
    benchmark::DoNotOptimize(&h);             \
  }                                           \
  state.SetItemsProcessed(N * iterations)

static void BM_MurmurHash2_64(benchmark::State& state) {
  PREAMBLE {
    h = MurmurHash2_64(&vals[i], sizeof(int64_t) * STRIDE, h);
  }
  EPILOG;
}

static void BM_XXH64(benchmark::State& state) {
  PREAMBLE {
    h = XXH64(&vals[i], sizeof(int64_t) * STRIDE, h);
  }
  EPILOG;
}

static void BM_XXH64_align(benchmark::State& state) {
  PREAMBLE {
    h = XXH64_endian_align_nong(&vals[i], sizeof(int64_t) * STRIDE, h);
  }
  EPILOG;
}

static void BM_Murmur3(benchmark::State& state) {
  PREAMBLE {
    MurmurHash3_x86_32(&vals[i], sizeof(int64_t) * STRIDE, h, &h);
  }
  EPILOG;
}

BENCHMARK(BM_Murmur3);
BENCHMARK(BM_MurmurHash2_64);
BENCHMARK(BM_XXH64);
BENCHMARK(BM_XXH64_align);

BENCHMARK_MAIN();
