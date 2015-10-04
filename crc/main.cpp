#include <nmmintrin.h>

int CrcHash(const void* data, int bytes, int hash) {
  int words = bytes / 4;
  bytes = bytes % 4;

  const int* p = reinterpret_cast<const int*>(data);
  while (words--) {
    hash = _mm_crc32_u32(hash, *p);
    ++p;
  }

  const char* s = reinterpret_cast<const char*>(p);
  while (bytes--) {
    hash = _mm_crc32_u8(hash, *s);
    ++s;
  }

  return hash;
} 

/*
int main(int argc, char** argv) {
  const char* c = "askdljfalsfjalsdkjfasldkfj";
  int len = strlen(c);

  uint32_t hash = 0;
  for (int j = 0; j < 10000; ++j) {
    for (int i = 0; i < 10000; ++i) {
      hash = CrcHash(c, len, hash);
    }
  }

  printf("%x\n", hash);
  return 0;
}
*/
