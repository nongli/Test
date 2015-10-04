#include <stdio.h>
#include <iostream>
#include <map>
#include <string>
#include <typeinfo>

#include "object-tracker.h"

using namespace std;

class Foo {
 public:
  int x;
};

class Foo1 ENABLE_OBJECT_TRACKING(Foo1)
};

class Bar ENABLE_OBJECT_TRACKING(Bar)
 public:
  Bar(int x) : x(x) {}
  int x;
};

INIT_OBJECT_TRACKING(Foo1);
INIT_OBJECT_TRACKING(Bar);

int main(int argc, char** argv) {
  printf("sizeof(Foo) = %ld\n", sizeof(Foo));
  printf("sizeof(Foo1) = %ld\n", sizeof(Foo1));
  printf("sizeof(Bar) = %ld\n", sizeof(Bar));

  Bar b1(1);
  Foo1 f1;
  Foo1 f2;
  cout << ObjectTracker::DumpObjects();
  {
    Bar b2(2);
    cout << ObjectTracker::DumpObjects();
  }
  cout << ObjectTracker::DumpObjects();
}

ObjectTracker::ObjectRefCount* ObjectTracker::objects_;

