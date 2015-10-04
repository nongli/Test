#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <iostream>
#include <vector>

using namespace std;

static const unsigned long FVN_PRIME = 1099511628211UL;
static const unsigned long FVN_SEED = 14695981039346656037UL;

static long FvnHash(const void* data, int bytes, long hash) {
  const char* ptr = reinterpret_cast<const char*>(data);
  while (bytes--) {
    hash = (*ptr ^ hash) * FVN_PRIME;
    ++ptr;
  }
  return hash;
}

// Implementation of Hyperloglog with some improvements published
// by google
long HLL(const vector<long>& values) {
  const int b = 10;
  const int m = pow(2, b);
  float alpha = 0;
  if (m == 16) {
    alpha = 0.673f;
  } else if (m == 32) {
    alpha = 0.697f;
  } else if (m == 64) {
    alpha = 0.709f;
  } else {
    alpha = 0.7213f / (1 + 1.079/m);
  }

  // Space usage is 2^b bytes.
  vector<char> registers;
  registers.resize(m);

  // Single pass through all the values
  for (long i = 0; i < values.size(); ++i) {
    unsigned long hash = FvnHash(&values[i], sizeof(long), FVN_SEED);
    if (hash != 0) {
      int register_idx = hash % m;
      char run = __builtin_ctzl(hash >> b) + 1;
      registers[register_idx] = ::max(registers[register_idx], run);
    }
  }

  // Aggregate registers into estimate
  float harmonic_mean = 0;
  for (int i = 0; i < registers.size(); ++i) {
    harmonic_mean += powf(2.0f, -registers[i]);
  }
  harmonic_mean = 1.0f / harmonic_mean;
  long estimate = (long)(alpha * m * m * harmonic_mean);

  // Correct for small cardinality error
  // TODO

  // Use linear counting if the cardinality is too low
  int num_zero_registers = 0;
  for (int i = 0; i < registers.size(); ++i) {
    if (registers[i] == 0) ++num_zero_registers;
  }
  if (num_zero_registers == 0) {
    return estimate;
  } else {
    return m * log((float)m / num_zero_registers);
  }

  return estimate;
}

long Distinct(vector<long>& values) {
  sort(values.begin(), values.end());
  long num = 1;
  long prev = values[0];
  for (long i = 1; i < values.size(); ++i) {
    if (values[i] != prev) {
      prev = values[i];
      ++num;
    }
  }
  return num;
}

int main(int argc, char** argv) {
  srand(time(0));

  const long N = 1000000L;
  cout << "Num values: " << N << endl;

  for (long max = 1; max < 1000L * 1000L * 1000L * 1000L; max *= 5) {
    vector<long> values;
    for (long i = 0; i < N; ++i) {
      values.push_back(rand() % max);
    }

    long estimate = HLL(values);
    long num_distinct = Distinct(values);
    float error = (estimate - num_distinct) * 100 / (float)num_distinct;

    cout << "Num Distinct: " << num_distinct << endl;
    cout << "Num Distinct Estimate: " << estimate << "(" << error << "%)" << endl << endl;
  }
  return 0;
}
