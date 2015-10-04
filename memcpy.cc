#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv) {
  char a[256];
  char b[256];
  int r = rand() % 10;
  memcpy(a, b, -r);
}
