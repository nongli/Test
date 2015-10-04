#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <time.h>

#include <boost/cstdint.hpp>

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

int main(int argc, char** argv) {
  const int iters = 30;
  const int N = 1000 * 1000;
  const int stride = 2;
  vector<int64_t> vals;
  vals.resize(N);

  for (int i = 0; i < N; ++i) {
    vals[i] = rand();
  }

  timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  unsigned long long h = 0;
  for (int i = 0; i < iters; ++i) {
    for (int j = 0; j < N; j += stride) {
      if (FN == MURMUR2) {
        h = MurmurHash2_64(&vals[j], sizeof(int64_t) * stride, h);
      } else if (FN == XXHASH) {
        h = XXH64(&vals[j], sizeof(int64_t) * stride, h);
      } else if (FN == XXHASH_NONG) {
        h = XXH64_endian_align_nong(&vals[j], sizeof(int64_t) * stride, h);
      } else if (FN == MURMUR3) {
        MurmurHash3_x86_32(&vals[j], sizeof(int64_t) * stride, h, &h);
      }
    }
  }

  clock_gettime(CLOCK_MONOTONIC, &end);
  int64_t nanos =
    (end.tv_sec - start.tv_sec) * 1000L * 1000L * 1000L +
    (end.tv_nsec - start.tv_nsec);

  double rate = (iters * N) * 1000. / nanos;
  switch (FN) {
    case MURMUR2:
      printf("Murmur2 ");
      break;
    case XXHASH:
      printf("XXHASH ");
      break;
    case XXHASH_NONG:
      printf("XXHASH_NONG ");
      break;
    case MURMUR3:
      printf("MURMUR3 ");
      break;
  }
  printf("Rate: %f million values/second\n", rate);
  return 0;
}
