#include <malloc.h>
#include <stdlib.h>
#include <string>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

const int kFromBufSize = 16 * 1024 * 1024;
const int kNumSlices = 20000;

struct Slice {
  char* ptr;
  int size;
};
inline void inline_memcpy(char* dst, const char* src, int size) {
  while (size--) {
    *dst++ = *src++;
  }
}


inline void sloppy_memcpy(char* dst, const char* src, int size) {
  while (size > 0) {
    memcpy(dst, src, 8);
    src += 8;
    dst += 8;
    size -= 8;
  }
}

// The standard memcpy operation is slow for variable small sizes.
// This implementation inlines the optimal realization for sizes 1 to 16.
// To avoid code bloat don't use it in case of not performance-critical spots,
// nor when you don't expect very frequent values of size <= 16.
inline void memcpy_inlined(void *dst, const void *src, size_t size) {
  // Compiler inlines code with minimal amount of data movement when third
  // parameter of memcpy is a constant.
  switch (size) {
    case  1: memcpy(dst, src, 1); break;
    case  2: memcpy(dst, src, 2); break;
    case  3: memcpy(dst, src, 3); break;
    case  4: memcpy(dst, src, 4); break;
    case  5: memcpy(dst, src, 5); break;
    case  6: memcpy(dst, src, 6); break;
    case  7: memcpy(dst, src, 7); break;
    case  8: memcpy(dst, src, 8); break;
    case  9: memcpy(dst, src, 9); break;
    case 10: memcpy(dst, src, 10); break;
    case 11: memcpy(dst, src, 11); break;
    case 12: memcpy(dst, src, 12); break;
    case 13: memcpy(dst, src, 13); break;
    case 14: memcpy(dst, src, 14); break;
    case 15: memcpy(dst, src, 15); break;
    case 16: memcpy(dst, src, 16); break;
    default: memcpy(dst, src, size); break;
  }
}


int main(int argc, char* argv[]) {
  char* from_buf = (char*)malloc(kFromBufSize);
  for (int i = 0; i < kFromBufSize; i++) {
    from_buf[i] = i;
  }

  Slice* slices = new Slice[kNumSlices];
  for (int i = 0; i < kNumSlices; i++) {
    slices[i].ptr = from_buf + ((random() % (kFromBufSize - 10))&(~7) );
    slices[i].size = 1 + (random() % 10);
  }

  char* out_buf = (char*)malloc(10 * kNumSlices);
  for (int n = 0; n < 25000; n++) {
    char* dst = out_buf;
    for (int i = 0; i < kNumSlices; i++) {
      //__builtin_prefetch(slices[i+PREFETCH_DISTANCE].ptr);
      //inline_memcpy(dst, slices[i].ptr, slices[i].size);
      sloppy_memcpy(dst, slices[i].ptr, slices[i].size);
      //memcpy_inlined(dst, slices[i].ptr, slices[i].size);
      //memcpy(dst, slices[i].ptr, slices[i].size);
      //memcpy(dst, slices[i].ptr, 8);
      dst += slices[i].size;
    }
    /*
    int fd = open("/dev/null", O_WRONLY);
    write(fd, out_buf, dst - out_buf);
    close(fd);
    */
  }
  free(out_buf);
  return 0;
}
