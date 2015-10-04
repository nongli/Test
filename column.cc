#include <stdio.h>

enum Type {
  INT, CHAR
};

struct Column {
  Type type;
  union {
    int i;
    char c;
  };

  template<typename T>
  static Column Create(T t);

  template<typename T>
  T& value();

  template<typename T>
  T value() const;
};

template<> Column Column::Create<int>(int i) {
  Column c;
  c.type = INT;
  c.i = i;
  return c;
}
template<> int Column::value<int>() const {
  return i;
}
template<> int& Column::value<int>() {
  return i;
}


int main(int argc, char** argv) {
  Column i_col = Column::Create<int>(5);
  printf("%d\n", i_col.value<int>());
  i_col.value<int>() = 10;
  printf("%d\n", i_col.value<int>());
  printf("Hello.\n");
  return 0;
}
