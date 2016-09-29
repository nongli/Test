#include <stdio.h>
#include <string.h>
#include <vector>
#include <cstdint>

using namespace std;

static const uint64_t MURMUR_PRIME = 0xc6a4a7935bd1e995;
static const int MURMUR_R = 47;

#define SIMPLE_HASH 1

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
  vector<int> year;
  vector<int> month;
  vector<int> day;

  for (int i = 2000; i < 2200; i++) {
    year.push_back(i);
  }
  for (int i = 0; i < 12; i++) {
    month.push_back(i);
  }
  for (int i = 0; i < 30; i++) {
    day.push_back(i);
  }

  int NUM_PARTITIONS = 100;
  vector<vector<int> > partitions;
  partitions.resize(NUM_PARTITIONS);

  for (int i = 0; i < year.size(); i++) {
    for (int j = 0; j < month.size(); j++) {
      for (int k = 0; k < day.size(); k++) {

        uint64_t hash = 0;
#if SIMPLE_HASH
        hash = 31 * hash + year[i];
        hash = 31 * hash + month[j];
        hash = 31 * hash + day[k];
#else
        hash = MurmurHash2_64(&year[i], sizeof(int), hash);
        hash = MurmurHash2_64(&month[j], sizeof(int), hash);
        hash = MurmurHash2_64(&day[k], sizeof(int), hash);
#endif
        partitions[hash % NUM_PARTITIONS].push_back(hash);
      }
    }
  }


  int NUM_BUCKETS = 10;
  vector<int> buckets;
  buckets.resize(NUM_BUCKETS);
  const vector<int>& bucket_data = partitions[5];
  for (int i = 0; i < bucket_data.size(); i++) {
    int idx = bucket_data[i] % NUM_BUCKETS;
    if (idx < 0) idx = -idx;
    buckets[idx]++;
  }


  int max_bucket_size = 0;
  for (int i = 0; i < NUM_BUCKETS; i++) {
    if (buckets[i] > max_bucket_size) max_bucket_size = buckets[i];
  }

  int expected_per_bucket = bucket_data.size()/ NUM_BUCKETS;

  printf("Expected per bucket: %d\n", expected_per_bucket);
  printf("Max bucket: %d\n", max_bucket_size);

}
