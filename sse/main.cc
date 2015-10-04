
#include <nmmintrin.h>
#include <smmintrin.h>
#include <stdio.h>

int main(int argc, char** argv) {

  const char* a = "aaaaaaaaaaaaaaaa";
  const char* b = "bbcdabcdabcdaaab";

  __m128i xmm_a = _mm_loadu_si128(reinterpret_cast<__m128i*>((char*)a));
  __m128i xmm_b = _mm_loadu_si128(reinterpret_cast<__m128i*>((char*)b));

  const int mode = _SIDD_CMP_EQUAL_ANY | _SIDD_UBYTE_OPS;

  int stra = _mm_cmpistra(xmm_a, xmm_b, mode);
  int strc = _mm_cmpistrc(xmm_a, xmm_b, mode);
  int stri = _mm_cmpistri(xmm_a, xmm_b, mode);
  int strm = _mm_extract_epi16(_mm_cmpistrm(xmm_a, xmm_b, mode), 0);
  int strs = _mm_cmpistrs(xmm_a, xmm_b, mode);
  int strz = _mm_cmpistrz(xmm_a, xmm_b, mode);

  printf("_mm_cmpistra: %d\n", stra);
  printf("_mm_cmpistrc: %d\n", strc);
  printf("_mm_cmpistri: %d\n", stri);
  printf("_mm_cmpistrm: ");
  for (int i = 0; i < 16; i++) {
    if (strm & (1 << i)) printf("1");
    else printf("0");
  }
  printf("\n");
  printf("_mm_cmpistrs: %d\n", strs);
  printf("_mm_cmpistrz: %d\n", strz);

  return 0;
}
