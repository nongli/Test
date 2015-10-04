#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <boost/cstdint.hpp>

using namespace std;


struct Record {
  bool flag;
  float val;
  int64_t id;
};

uint64_t Rdtsc() {
  uint32_t lo, hi;
  __asm__ __volatile__ (      
    "xorl %%eax,%%eax \n        cpuid"
    ::: "%rax", "%rbx", "%rcx", "%rdx");
  __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
  return (uint64_t)hi << 32 | lo;
}

void Init(vector<Record>* records, int num) {
  records->resize(num);

  for (int i = 0; i < num; ++i) {
    (*records)[i].flag = rand() % 4 != 0;
    (*records)[i].flag = 1;
    (*records)[i].val = rand() / (double)RAND_MAX * 10;
    (*records)[i].id = i;
  }
}

#define DECLARE_VARS_IMPL(count_name, sum_name, avg_name, max_name)\
  int64_t count_name = 0;\
  int64_t sum_name = 0;\
  float avg_name = 0;\
  float max_name = 0

#define DECLARE_VARS(id) DECLARE_VARS_IMPL(count##id, sum##id, avg##id, max##id)

#define PROCESS_RECORD_IMPL2(record, count_dst, sum_dst, avg_dst, max_dst)\
  if (record.flag) {\
    ++count_dst;\
    sum_dst += record.id;\
    avg_dst += record.val;\
    max_dst = std::max(max_dst, record.val);\
  }

#define PROCESS_RECORD_IMPL(record, count_dst, sum_dst, avg_dst, max_dst)\
  if (1) {\
    ++count_dst;\
    avg_dst += record.val;\
  }

#define PROCESS_RECORD(record, id) \
  PROCESS_RECORD_IMPL(record, count##id, sum##id, avg##id, max##id)

__attribute__((noinline))
void Test(const vector<Record>& records, int64_t* count, int64_t* sum, float* avg, float* max) {
  int num = records.size();
  *count = 0;
  *sum = 0;
  *avg = 0;
  *max = 0;
  
  DECLARE_VARS(1);

  for (int i = 0; i < num; ++i) {
    PROCESS_RECORD(records[i], 1);
  }

  *count = count1;
  *sum = sum1;
  *avg = avg1 / *count;
  *max = max1;
}

enum Type {
  FLOAT,
  INT
};

#define TYPES_IMPL(count, avg, index) \
  if (1) { \
    ++count; \
    for (int j = 0; j < num_exprs; ++j) { \
      switch (val_type[j]) { \
        case FLOAT: \
          avg[j] += *(float*)(record_buffer + record_byte_size * index + val_offset[j]); \
          break; \
        case INT: \
          avg[j] += *(int32_t*)(record_buffer + record_byte_size * index + val_offset[j]); \
          break; \
      } \
    } \
  }

__attribute__((noinline))
void TestTypes(char* record_buffer, int num, int num_exprs, int record_byte_size, int flag_offset, 
    int* val_offset, Type* val_type, int64_t* count, float* avg) {
  *count = 0;
  memset(avg, 0, sizeof(float) * num_exprs);
  
  for (int i = 0; i < num; ++i) {
    TYPES_IMPL(*count, avg, 0);
    record_buffer += record_byte_size;
  }
  *avg /= *count;
}

__attribute__((noinline))
void TestTypes2(char* record_buffer, int num, int num_exprs, int record_byte_size, int flag_offset, 
    int* val_offset, Type* val_type, int64_t* count, float* avg) {
  *count = 0;

  int64_t count1 = 0, count2 = 0;
  float a1[num_exprs];
  float a2[num_exprs];
  memset(a1, 0, sizeof(float) * num_exprs);
  memset(a2, 0, sizeof(float) * num_exprs);
  
  for (int i = 0; i < num; i += 2) {
    TYPES_IMPL(count1, a1, 0);
    TYPES_IMPL(count2, a2, 1);
    record_buffer += record_byte_size * 2;
  }
  *count = count1 + count2;
  for (int j = 0; j < num_exprs; ++j) {
    avg[j] = (a1[j] + a2[j]) / *count;
  }
}

__attribute__((noinline))
void TestTypes4(char* record_buffer, int num, int num_exprs, int record_byte_size, int flag_offset, 
    int* val_offset, Type* val_type, int64_t* count, float* avg) {
  *count = 0;

  int64_t count1 = 0, count2 = 0, count3 = 0, count4 = 0;
  float a1[num_exprs];
  float a2[num_exprs];
  float a3[num_exprs];
  float a4[num_exprs];
  memset(a1, 0, sizeof(float) * num_exprs);
  memset(a2, 0, sizeof(float) * num_exprs);
  memset(a3, 0, sizeof(float) * num_exprs);
  memset(a4, 0, sizeof(float) * num_exprs);
  
  for (int i = 0; i < num; i += 4) {
    TYPES_IMPL(count1, a1, 0);
    TYPES_IMPL(count2, a2, 1);
    TYPES_IMPL(count3, a3, 2);
    TYPES_IMPL(count4, a4, 3);
    record_buffer += record_byte_size * 4;
  }
  *count = count1 + count2 + count3 + count4;
  for (int j = 0; j < num_exprs; ++j) {
    avg[j] = (a1[j] + a2[j] + a3[j] + a4[j]) / *count;
  }
}

typedef void* (*Expr)(void* val, Type type);

void* Identity(void* val, Type type) {
  return val;
}

#define EXPRS_IMPL(count, avg, index) \
  if (1) { \
    ++count; \
    for (int j = 0; j < num_exprs; ++j) { \
      switch (val_type[j]) { \
        case FLOAT: \
          avg[j] += *(float*)expr[j](record_buffer + record_byte_size * index + val_offset[j], val_type[j]); \
          break; \
        case INT: \
          avg[j] += *(int32_t*)expr[j](record_buffer + record_byte_size * index + val_offset[j], val_type[j]); \
          break; \
      } \
    } \
  }

__attribute__((noinline))
void TestExprs(char* record_buffer, int num, int num_exprs, int record_byte_size, int flag_offset, 
    int* val_offset, Type* val_type, Expr* expr, int64_t* count, float* avg) {
  *count = 0;
  memset(avg, 0, sizeof(float) * num_exprs);
  
  for (int i = 0; i < num; ++i) {
    EXPRS_IMPL(*count, avg, 0);
    record_buffer += record_byte_size;
  }
  *avg /= *count;
}

__attribute__((noinline))
void TestExprs2(char* record_buffer, int num, int num_exprs, int record_byte_size, int flag_offset, 
    int* val_offset, Type* val_type, Expr* expr, int64_t* count, float* avg) {
  *count = 0;

  int64_t count1 = 0, count2 = 0;
  float a1[num_exprs];
  float a2[num_exprs];
  memset(a1, 0, sizeof(float) * num_exprs);
  memset(a2, 0, sizeof(float) * num_exprs);
  
  for (int i = 0; i < num; i += 2) {
    EXPRS_IMPL(count1, a1, 0);
    EXPRS_IMPL(count2, a2, 1);
    record_buffer += record_byte_size * 2;
  }
  *count = count1 + count2;
  for (int j = 0; j < num_exprs; ++j) {
    avg[j] = (a1[j] + a2[j]) / *count;
  }
}

__attribute__((noinline))
void TestExprs4(char* record_buffer, int num, int num_exprs, int record_byte_size, int flag_offset, 
    int* val_offset, Type* val_type, Expr* expr, int64_t* count, float* avg) {
  *count = 0;

  int64_t count1 = 0, count2 = 0, count3 = 0, count4 = 0;
  float a1[num_exprs];
  float a2[num_exprs];
  float a3[num_exprs];
  float a4[num_exprs];
  memset(a1, 0, sizeof(float) * num_exprs);
  memset(a2, 0, sizeof(float) * num_exprs);
  memset(a3, 0, sizeof(float) * num_exprs);
  memset(a4, 0, sizeof(float) * num_exprs);
  
  for (int i = 0; i < num; i += 4) {
    EXPRS_IMPL(count1, a1, 0);
    EXPRS_IMPL(count2, a2, 1);
    EXPRS_IMPL(count3, a3, 2);
    EXPRS_IMPL(count4, a4, 3);
    record_buffer += record_byte_size * 4;
  }
  *count = count1 + count2 + count3 + count4;
  for (int j = 0; j < num_exprs; ++j) {
    avg[j] = (a1[j] + a2[j] + a3[j] + a4[j]) / *count;
  }
}

__attribute__((noinline))
void Test2(const vector<Record>& records, int64_t* count, int64_t* sum, float* avg, float* max) {
  int num = records.size();
  *count = 0;
  *sum = 0;
  *avg = 0;

  DECLARE_VARS(1);
  DECLARE_VARS(2);

  for (int i = 0; i < num; i += 2) {
    PROCESS_RECORD(records[i], 1);
    PROCESS_RECORD(records[i + 1], 2);
  }

  *count = count1 + count2;
  *sum = sum1 + sum2;
  *avg = (avg1 + avg2) / *count;
  *max = std::max(max1, max2);
}

__attribute__((noinline))
void Test4(const vector<Record>& records, int64_t* count, int64_t* sum, float* avg, float* max) {
  int num = records.size();
  *count = 0;
  *sum = 0;
  *avg = 0;

  DECLARE_VARS(1);
  DECLARE_VARS(2);
  DECLARE_VARS(3);
  DECLARE_VARS(4);

  for (int i = 0; i < num; i += 4) {
    PROCESS_RECORD(records[i], 1);
    PROCESS_RECORD(records[i + 1], 2);
    PROCESS_RECORD(records[i + 2], 3);
    PROCESS_RECORD(records[i + 3], 4);
  }

  *count = count1 + count2 + count3 + count4;
  *sum = sum1 + sum2 + sum3 + sum4;
  *avg = (avg1 + avg2 + avg3 + avg4) / *count;
  *max = std::max(std::max(max1, max2), std::max(max3, max4));
}

__attribute__((noinline))
void Test8(const vector<Record>& records, int64_t* count, int64_t* sum, float* avg, float* max) {
  int num = records.size();
  *count = 0;
  *sum = 0;
  *avg = 0;

  DECLARE_VARS(1);
  DECLARE_VARS(2);
  DECLARE_VARS(3);
  DECLARE_VARS(4);
  DECLARE_VARS(5);
  DECLARE_VARS(6);
  DECLARE_VARS(7);
  DECLARE_VARS(8);

  for (int i = 0; i < num; i += 8) {
    PROCESS_RECORD(records[i], 1);
    PROCESS_RECORD(records[i + 1], 2);
    PROCESS_RECORD(records[i + 2], 3);
    PROCESS_RECORD(records[i + 3], 4);
    PROCESS_RECORD(records[i + 4], 5);
    PROCESS_RECORD(records[i + 5], 6);
    PROCESS_RECORD(records[i + 6], 7);
    PROCESS_RECORD(records[i + 7], 8);
  }

  *count = count1 + count2 + count3 + count4 + count5 + count6 + count7 + count8;
  *sum = sum1 + sum2 + sum3 + sum4 + sum5 + sum6 + sum7 + sum8;
  *avg = (avg1 + avg2 + avg3 + avg4 + avg5 + avg6 + avg7 + avg8) / *count;
  *max = std::max(std::max(max1, max2), std::max(max3, max4));
  *max = std::max(*max, std::max(std::max(max5, max6), std::max(max7, max8)));
}

int main(int argc, char** argv) {
  int flag = 1;
  int num = 1024 * 5;
  int iters = 1000 * 1000;
  
  int exprs = 1;
  int flag_offset = 0;
  int record_size = sizeof(Record);
  Expr expr = Identity;

  if (argc == 2) flag = atoi(argv[1]);
  if (argc == 1000) {
    // Dummy code to prevent compiler from optimizing out the offsets
    exprs = 2;
    flag_offset = 10;
    record_size = 1;
    expr = NULL;
  }

  vector<Record> records;
  Init(&records, num);

  int64_t total = num * iters;

  int64_t count, sum;
  float avg, max;
  if (flag & 1 || flag == 0) {
    uint64_t start = Rdtsc();
    for (int i = 0; i < iters; ++i) Test(records, &count, &sum, &avg, &max);
    uint64_t cycles = Rdtsc() - start;
    cout << "Unroll 1: cycles per record: " << (double)cycles / total << endl;
  }
  if (flag & 2 || flag == 0) {
    uint64_t start = Rdtsc();
    for (int i = 0; i < iters; ++i) Test2(records, &count, &sum, &avg, &max);
    uint64_t cycles = Rdtsc() - start;
    cout << "Unroll 2: cycles per record: " << (double)cycles / total << endl;
  }
  if (flag & 4 || flag == 0) {
    uint64_t start = Rdtsc();
    for (int i = 0; i < iters; ++i) Test4(records, &count, &sum, &avg, &max);
    uint64_t cycles = Rdtsc() - start;
    cout << "Unroll 4: cycles per record: " << (double)cycles / total << endl;
  }
  if (flag & 8 || flag == 0) {
    uint64_t start = Rdtsc();
    for (int i = 0; i < iters; ++i) Test8(records, &count, &sum, &avg, &max);
    uint64_t cycles = Rdtsc() - start;
    cout << "Unroll 8: cycles per record: " << (double)cycles / total << endl;
  }
  vector<int> offsets;
  offsets.push_back(4);
  vector<Type> types;
  types.push_back(FLOAT);
  if (flag & 16 || flag == 0) {
    uint64_t start = Rdtsc();
    for (int i = 0; i < iters; ++i) {
      TestTypes((char*)&records[0], num, exprs, record_size, flag_offset, &offsets[0], &types[0], &count, &avg);
    }
    uint64_t cycles = Rdtsc() - start;
    cout << "Types: cycles per record: " << (double)cycles / total << endl;
  }
  if (flag & 32 || flag == 0) {
    uint64_t start = Rdtsc();
    for (int i = 0; i < iters; ++i) {
      TestTypes2((char*)&records[0], num, exprs, record_size, flag_offset, &offsets[0], &types[0], &count, &avg);
    }
    uint64_t cycles = Rdtsc() - start;
    cout << "Types Unroll 2: cycles per record: " << (double)cycles / total << endl;
  }
  if (flag & 32 || flag == 0) {
    uint64_t start = Rdtsc();
    for (int i = 0; i < iters; ++i) {
      TestTypes4((char*)&records[0], num, exprs, record_size, flag_offset, &offsets[0], &types[0], &count, &avg);
    }
    uint64_t cycles = Rdtsc() - start;
    cout << "Types Unroll 4: cycles per record: " << (double)cycles / total << endl;
  }
  if (flag & 128 || flag == 0) {
    uint64_t start = Rdtsc();
    for (int i = 0; i < iters; ++i) {
      TestExprs((char*)&records[0], num, exprs, record_size, flag_offset, &offsets[0], &types[0], &expr, &count, &avg);
    }
    uint64_t cycles = Rdtsc() - start;
    cout << "Exprs: cycles per record: " << (double)cycles / total << endl;
  }
  if (flag & 256 || flag == 0) {
    uint64_t start = Rdtsc();
    for (int i = 0; i < iters; ++i) {
      TestExprs2((char*)&records[0], num, exprs, record_size, flag_offset, &offsets[0], &types[0], &expr, &count, &avg);
    }
    uint64_t cycles = Rdtsc() - start;
    cout << "Exprs Unroll 2: cycles per record: " << (double)cycles / total << endl;
  }
  if (flag & 512 || flag == 0) {
    uint64_t start = Rdtsc();
    for (int i = 0; i < iters; ++i) {
      TestExprs4((char*)&records[0], num, exprs, record_size, flag_offset, &offsets[0], &types[0], &expr, &count, &avg);
    }
    uint64_t cycles = Rdtsc() - start;
    cout << "Exprs Unroll 2: cycles per record: " << (double)cycles / total << endl;
  }

  cout << "Count: " << count << endl;
  cout << "Sum: " << sum << endl;
  cout << "Avg: " << avg << endl;
  cout << "Max: " << max << endl;
}
