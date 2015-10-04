#include "main.h"
#include <dlfcn.h>

void A::Foo() {
  printf("A::Foo\n");
}

typedef A*(*CreateB)();

int main(int argc, char** argv) {
  
  void* h = dlopen("./test.so", RTLD_LAZY);
  if (h == NULL) return -1;
  CreateB f = (CreateB)dlsym(h, "CreateB");
  if (f == NULL) return -1;

  A a;
  A* b = f();
  a.Foo();
  b->Foo();
  a.Boo();
  b->Boo();
  printf("Done.\n");
  return 0;
}
