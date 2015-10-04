#ifndef ABC
#define ABC
#include <stdio.h>

class A {
 public:
  virtual void Foo();
  virtual void Boo() {
    printf("A::Boo\n");
  }
};

#endif

