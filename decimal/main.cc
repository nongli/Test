#include <float.h>
#include <stdio.h>
#include <math.h>
#include <sstream>
#include <iostream>
#include <boost/cstdint.hpp>

using namespace std;

struct DecimalDesc {
  int precision;
  int scale;

  DecimalDesc(int precision = 8, int scale = 0) : precision(precision), scale(scale) {
  }
};

// Decimal class that supports up to 32 bits.
// The underlying storage can store between:
// [-2147483648, 2147483647], so it is safe to use this
// type to store up to 9 digits of precision.
class Decimal32 {
 public:
  Decimal32() : val_(0) {}
  Decimal32(int32_t val) : val_(val) {}

  // Assumes this and other have the same scale
  Decimal32& operator+=(const Decimal32& other) { val_ += other.val_; }
  const Decimal32 operator+(const Decimal32& y) const {
    return Decimal32(val_ + y.val_);
  }

  bool operator==(const Decimal32& other) { return val_ == other.val_; }

  float ToFloat(const DecimalDesc& desc) const {
    float result = val_;
    int scale = desc.scale;
    while (scale < 0) {
      result /= 10;
      ++scale;
    }
    while (scale > 0) {
      result *= 10;
      --scale;
    }
    return result;
  }

  string ToString(const DecimalDesc& desc) const {
    stringstream ss;
    if (desc.scale == 0) {
      ss << val_;
    } else if (desc.scale > 0) {
      ss << val_;
      for (int i = 0; i < desc.scale; ++i) {
        ss << '0';
      }
    } else {
      int32_t scale = pow(10, -desc.scale);
      int32_t dec = val_ / scale;
      int32_t rem = val_ % scale;
      ss << dec;
      ss << ".";
      if (rem == 0) {
        for (int i = 0; i < -desc.scale; ++i) {
          ss << "0";
        }
      } else {
        int32_t leading_zeros_val = rem * 10;
        while (leading_zeros_val < scale) {
          ss << "0";
          leading_zeros_val *= 10;
        }
        ss << rem;
      }
    }
    return ss.str();
  }

 private:
  // Underlying storage, note that the precision and scale is specified outside
  // this class and must be supplied by the caller in the various functions
  // to minimize the size of this object.
  int32_t val_;
};

int main(int argc, char** argv) {
  Decimal32 decimal(123000);
  DecimalDesc desc_8_0;
  DecimalDesc desc_8_3(8, 3);
  DecimalDesc desc_8_m3(8, -3);

  printf("%f vs. %s\n", decimal.ToFloat(desc_8_0), decimal.ToString(desc_8_0).c_str());
  printf("%f vs. %s\n", decimal.ToFloat(desc_8_3), decimal.ToString(desc_8_3).c_str());
  printf("%f vs. %s\n", decimal.ToFloat(desc_8_m3), decimal.ToString(desc_8_m3).c_str());

  Decimal32 x(11); // 1.1
  Decimal32 y(22); // 2.2
  DecimalDesc desc_8_m1(8, -1);
  Decimal32 result = x + y;
  printf("%f vs. %s\n", result.ToFloat(desc_8_m1), result.ToString(desc_8_m1).c_str());

  return 0;
}
