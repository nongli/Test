#include "library.h"

class B : public A {
  virtual void Foo() {
    printf("B::Foo\n");
  }
};

extern "C" A* CreateB() {
  return new B;
}
