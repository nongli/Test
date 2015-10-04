// Copyright (c) 2012 Cloudera, Inc. All rights reserved.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include "runtime/string-value.h"
#include "util/benchmark.h"
#include "util/string-parser.h"

#include <nmmintrin.h>
#include <smmintrin.h>

using namespace impala;
using namespace std;

#define VALIDATE 1

struct TestData {
  vector<StringValue> data;
  vector<string> memory;
  vector<double> result;
};

void AddTestData(TestData* data, const string& input) {
  data->memory.push_back(input);
  const string& str = data->memory.back();
  data->data.push_back(StringValue(const_cast<char*>(str.c_str()), str.length()));
}

void AddTestData(TestData* data, int N, double min = -10, double max = 10) {
  for (int i = 0; i < N; ++i) {
    double val = rand();
    val /= RAND_MAX;
    val = (val * (max - min)) + min;
    stringstream ss;
    ss << val;
    AddTestData(data, ss.str());
  }
}

void TestAtof(int batch_size, void* d) {
  TestData* data = reinterpret_cast<TestData*>(d);
  for (int i = 0; i < batch_size; ++i) {
    int N = data->data.size();
    for (int j = 0; j < N; ++j) {
      data->result[j] = atof(data->data[j].ptr);
    }
  }
}

void TestImpala(int batch_size, void* d) {
  TestData* data = reinterpret_cast<TestData*>(d);
  for (int i = 0; i < batch_size; ++i) {
    int N = data->data.size();
    for (int j = 0; j < N; ++j) {
      const StringValue& str = data->data[j];
      StringParser::ParseResult dummy;
      double val = StringParser::StringToFloat<double>(str.ptr, str.len, &dummy);
#if VALIDATE
      // Use fabs?
      if (val != data->result[j]) {
        cout << "Incorrect result.  Expected: " 
             << data->result[j] << ". Parsed: " << val << endl;
        exit(-1);
      }
#endif
      data->result[j] = val;
    }
  }
}

void TestStrtod(int batch_size, void* d) {
  TestData* data = reinterpret_cast<TestData*>(d);
  for (int i = 0; i < batch_size; ++i) {
    int N = data->data.size();
    for (int j = 0; j < N; ++j) {
      data->result[j] = strtod(data->data[j].ptr, NULL);
    }
  }
}
template <typename T>
inline T StringToFloat2(const char* s, int len, StringParser::ParseResult* result) {
  // Use double here to not lose precision while accumulating the result
  double val = 0;
  bool negative = false;
  int i = 0;
  int divide = 1;
  int magnitude = 1;
  switch (*s) {
    case '-': negative = true;
    case '+': i = 1;
  }
  for (; i < len; ++i) {
    /*
    if (LIKELY(s[i] >= '0' && s[i] <= '9')) {
      val = val * 10 + s[i] - '0';
      divide *= magnitude;
    } else if (s[i] == '.') {
      magnitude = 10;
    } 
    */
    if (UNLIKELY(s[i] == '.')) {
      magnitude = 10;
    } else {
      val = val * 10 + s[i] - '0';
      divide *= magnitude;
    }
    /*else if (s[i] == 'e' || s[i] == 'E') {
      break;
    } else {
      *result = StringParser::PARSE_FAILURE;
      return 0;
    }
    */
  }

  val /= divide;

  if (i < len && (s[i] == 'e' || s[i] == 'E')) {
    ++i;
    int exp = StringParser::StringToInt<int>(s + i, len - i, result);
    if (UNLIKELY(*result != StringParser::PARSE_SUCCESS)) return 0;
    while (exp > 0) {
      val *= 10;
      exp--;
    }
    while (exp < 0) {
      val *= 0.1;
      exp++;
    }
  }

  // Determine if it is an overflow case and update the result
  if (UNLIKELY(val == std::numeric_limits<T>::infinity())) {
    *result = StringParser::PARSE_OVERFLOW;
  } else {
    *result = StringParser::PARSE_SUCCESS;
  }
  return (T)(negative ? -val : val);
}

void TestImpala2(int batch_size, void* d) {
  TestData* data = reinterpret_cast<TestData*>(d);
  for (int i = 0; i < batch_size; ++i) {
    int N = data->data.size();
    for (int j = 0; j < N; ++j) {
      const StringValue& str = data->data[j];
      StringParser::ParseResult dummy;
      double val = StringToFloat2<double>(str.ptr, str.len, &dummy);
#if VALIDATE
      // Use fabs?
      if (val != data->result[j]) {
        cout << "Incorrect result.  Expected: " 
             << data->result[j] << ". Parsed: " << val << endl;
        exit(-1);
      }
#endif
      data->result[j] = val;
    }
  }
}

inline int64_t StringToIntHelper(const char* s, int len) {
  int64_t val = 0;
  for (int i = 0; i < len; ++i) {
    int digit = s[i] - '0';
    val = val * 10 + digit;
  }
  return val;
}

char tmp[16] = {0};
__m128i xmm_search;

double NongAtof(const char* s, int len) {
  bool negative = false;
  switch (*s) {
    case '-': negative = true;
    case '+': ++s; --len;
  }

  double value = 0;

  if (UNLIKELY(len > 16)) {
    StringParser::ParseResult dummy;
    value = StringParser::StringToFloat<float>(s, len, &dummy);
  } else {

    //__m128i xmm_buf = _mm_loadu_si128(reinterpret_cast<__m128i*>((char*)s));

    //int dot = _mm_cmpestri(xmm_search, 1, xmm_buf, len, 0);
    int dot = 16;
    for (int i = 0; i < len; ++i) {
      if (UNLIKELY(s[i] == '.')) {
        dot = i;
        break;
      }
    }
    if (UNLIKELY(dot == 16)) {
      value = StringToIntHelper(s, len);
    } else {
      int64_t whole = 0;
      int64_t fractional = 0;
      whole = StringToIntHelper(s, dot);
      fractional = StringToIntHelper(s + dot + 1, len - dot - 1);
      int magnitude = 1;
      for (int i = 0; i < len - dot - 1; ++i) {
        magnitude *= 10;
      }
      value = whole + (double)fractional / magnitude;
    }
  }

  return negative ? -value : value;
}

void TestNong(int batch_size, void* d) {
  TestData* data = reinterpret_cast<TestData*>(d);
  for (int i = 0; i < batch_size; ++i) {
    int N = data->data.size();
    for (int j = 0; j < N; ++j) {
      const StringValue& str = data->data[j];
      double val = NongAtof(str.ptr, str.len);
#if VALIDATE
      // Use fabs?
      if (val != data->result[j]) {
        cout << "Incorrect result.  Expected: " 
             << data->result[j] << ". Parsed: " << val << endl;
        exit(-1);
      }
#endif
      data->result[j] = val;
    }
  }
}

int main(int argc, char **argv) {
  TestData data;
    
  tmp[0] = '.';
  xmm_search = _mm_loadu_si128(reinterpret_cast<__m128i*>((char*)tmp));

  // Most data is probably positive
  AddTestData(&data, 1000, -5, 1000);
  //AddTestData(&data, "1.1e12");

  data.result.resize(data.data.size());

  const char* s = "123.456";
  double val = NongAtof(s, strlen(s));
  cout << val << endl;
  
  // Run a warmup to iterate through the data.  
  TestAtof(100, &data);

  double strtod_rate = Benchmark::Measure(TestStrtod, &data);
  double atof_rate = Benchmark::Measure(TestAtof, &data);
  double impala_rate = Benchmark::Measure(TestImpala, &data);
  double impala2_rate = Benchmark::Measure(TestImpala2, &data);
  double nong_rate = Benchmark::Measure(TestNong, &data);

  cout << "Strtod Rate (per ms): " << strtod_rate << endl;
  cout << "Atof Rate (per ms): " << atof_rate << endl;
  cout << "Impala Rate (per ms): " << impala_rate << endl;
  cout << "Impala2 Rate (per ms): " << impala2_rate << endl;
  cout << "Nong Rate (per ms): " << nong_rate << endl;

  return 0;
}

