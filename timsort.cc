#include <stdio.h>
#include <assert.h>
#include <algorithm>
#include <vector>
#include <string>
#include <stdlib.h>
#include <string.h>

#include <boost/cstdint.hpp>

int64_t num_compares = 0;

struct Int {
  Int(int i = 0) : i(i) {}

  bool operator<(const Int& o) const {
    ++num_compares;
    return i < o.i;
  }
  bool operator<=(const Int& o) const {
    ++num_compares;
    return i <= o.i;
  }
  bool operator>(const Int& o) const {
    ++num_compares;
    return i > o.i;
  }
  bool operator>=(const Int& o) const {
    ++num_compares;
    return i >= o.i;
  }

  int i;
};

template <typename T>
class TimSort {
 public:
  static void Sort(T* a, int64_t n) {
    if (n < 2) return;

    int64_t lo = 0;
    int64_t hi = n;

    TimSort ts(a);
    if (n < MIN_MERGE) {
      int64_t run_len = ts.CountRunAndMakeAscending(lo, hi);
      ts.BinarySort(lo, hi, lo + run_len);
      return;
    }

    const int64_t min_run = MinRunLength(n);
    int64_t cur = lo;
    do {
      int64_t run_len = ts.CountRunAndMakeAscending(cur, hi);
      if (run_len < min_run) {
        const int64_t force = std::min(n, min_run);
        ts.BinarySort(cur, cur + force, cur + run_len);
        run_len = force;
      }
      ts.PushRun(cur, run_len);
      ts.MergeCollapse();

      cur += run_len;
      n -= run_len;
    } while (n != 0);

    assert(cur == hi);
    ts.MergeForceCollapse();
  }

 private:
  const static int MIN_MERGE = 32;
  const static int MIN_GALLOP = 7;

  TimSort(T* a) : a_(a), min_gallop_(MIN_GALLOP) { }

  static int MinRunLength(int64_t n) {
    int r = 0;
    while (n >= MIN_MERGE) {
      r |= (n & 1);
      n >>= 1;
    }
    return n + r;
  }

  void BinarySort(int64_t lo, int64_t hi, int64_t start) {
    if (start == lo) ++start;

    for (; start < hi; ++start) {
      T pivot = a_[start];
      int64_t left = lo;
      int64_t right = start;

      while (left < right) {
        int64_t mid = (left + right) >> 1;
        if (pivot < a_[mid]) {
          right = mid;
        } else {
          left = mid + 1;
        }
      }

      for (int64_t p = start; p > left; --p) {
        a_[p] = a_[p - 1];
      }
      a_[left] = pivot;
    }
  }

  void Reverse(int64_t lo, int64_t hi) {
    --hi;
    while (lo < hi) {
      T t = a_[lo];
      a_[lo++] = a_[hi];
      a_[hi--] = t;
    }
  }

  int64_t CountRunAndMakeAscending(int64_t lo, int64_t hi) {
    int64_t run_hi = lo + 1;
    if (run_hi == hi) return 1;

    if (a_[run_hi++] < a_[lo]) {
      // Descending
      while (run_hi < hi && a_[run_hi] < a_[run_hi - 1]) {
        ++run_hi;
      }
      // Reverse
      Reverse(lo, run_hi);
    } else {
      while (run_hi < hi && a_[run_hi] >= a_[run_hi - 1]) {
        ++run_hi;
      }
    }
    return run_hi - lo;
  }

  struct Run {
    int64_t start;
    int64_t len;
    Run(int64_t s, int64_t l) : start(s), len(l) {}
  };

  T* a_;
  std::vector<Run> pending_;
  std::vector<T> tmp_;
  int64_t min_gallop_;

  void PushRun(int64_t start, int64_t len) {
    pending_.push_back(Run(start, len));
  }

  void MergeCollapse() {
    while (pending_.size() > 1) {
      int n = pending_.size() - 2;
      if (n > 0 && pending_[n - 1].len < pending_[n].len + pending_[n + 1].len) {
        if (pending_[n - 1].len < pending_[n + 1].len) --n;
        MergeAt(n);
      } else if (pending_[n].len <= pending_[n + 1].len) {
        MergeAt(n);
      } else {
        break;
      }
    }
  }

  void MergeForceCollapse() {
    while (pending_.size() > 1) {
      int n = pending_.size() - 2;
      if (n > 0 && pending_[n - 1].len < pending_[n + 1].len) --n;
      MergeAt(n);
    }
  }

  void MergeAt(int n) {
    int64_t base1 = pending_[n].start;
    int64_t len1 = pending_[n].len;
    int64_t base2 = pending_[n + 1].start;
    int64_t len2 = pending_[n + 1].len;

    pending_[n].len = len1 + len2;
    if (n == pending_.size() - 3) pending_[n + 1] = pending_[n + 2];
    pending_.pop_back();

    int64_t k = GallopRight(a_[base2], a_, base1, len1, 0);
    base1 += k;
    len1 -= k;
    if (len1 == 0) return;

    len2 = GallopLeft(a_[base1 + len1 - 1], a_, base2, len2, len2 - 1);
    if (len2 == 0) return;

    if (len1 <= len2) {
      MergeLo(base1, len1, base2, len2);
    } else {
      MergeHi(base1, len1, base2, len2);
    }
  }

  static int64_t GallopRight(T key, T* a, int64_t base, int64_t len, int64_t hint) {
    int64_t last_ofs = 0;
    int64_t ofs = 1;
    if (key < a[base + hint]) {
      int64_t max_ofs = hint + 1;
      while (ofs < max_ofs && key < a[base + hint - ofs]) {
        last_ofs = ofs;
        ofs = (ofs << 1) + 1;
      }
      if (ofs > max_ofs) ofs = max_ofs;
      int64_t tmp = last_ofs;
      last_ofs = hint - ofs;
      ofs = hint - tmp;
    } else {
      int64_t max_ofs = len - hint;
      while (ofs < max_ofs && key >= a[base + hint + ofs]) {
        last_ofs = ofs;
        ofs = (ofs << 1) + 1;
        if (ofs > max_ofs) ofs = max_ofs;
      }
      last_ofs += hint;
      ofs += hint;
    }

    ++last_ofs;
    while (last_ofs < ofs) {
      int64_t m = last_ofs + ((ofs - last_ofs) >> 1);
      if (key < a[base + m]) {
        ofs = m;
      } else {
        last_ofs = m + 1;
      }
    }
    return ofs;
  }

  static int64_t GallopLeft(T key, T* a, int64_t base, int64_t len, int64_t hint) {
    int64_t last_ofs = 0;
    int64_t ofs = 1;
    if (key > a[base + hint]) {
      int64_t max_ofs = len - hint;
      while (ofs < max_ofs && key > a[base + hint + ofs]) {
        last_ofs = ofs;
        ofs = (ofs << 1) + 1;
      }
      if (ofs > max_ofs) ofs = max_ofs;
      last_ofs += hint;
      ofs += hint;
    } else {
      int64_t max_ofs = hint + 1;
      while (ofs < max_ofs && key <= a[base + hint - ofs]) {
        last_ofs = ofs;
        ofs = (ofs << 1) + 1;
        if (ofs > max_ofs) ofs = max_ofs;
      }
      int64_t tmp = last_ofs;
      last_ofs = hint - ofs;
      ofs = hint - tmp;
    }

    ++last_ofs;
    while (last_ofs < ofs) {
      int64_t m = last_ofs + ((ofs - last_ofs) >> 1);
      if (key > a[base + m]) {
        last_ofs = m + 1;
      } else {
        ofs = m;
      }
    }
    return ofs;
  }

  void MergeLo(int64_t base1, int64_t len1, int64_t base2, int64_t len2) {
    CopyToTmp(base1, len1);

    int64_t cursor1 = 0;                // Index into tmp
    int64_t cursor2 = base2;            // Index into a
    int64_t dest = base1;               // Index into a
    T* a = a_;
    T* tmp = &tmp_[0];

    // Move first element of second run and deal with degenerate cases
    a[dest++] = a[cursor2++];
    if (--len2 == 0) {
      memcpy(&a[dest], &tmp[cursor1], len1 * sizeof(T));
      return;
    }
    if (len1 == 1) {
      memcpy(&a[dest], &a[cursor2], len2 * sizeof(T));
      a[dest + len2] = tmp[cursor1]; // Last elt of run 1 to end of merge
      return;
    }

    int64_t min_gallop = min_gallop_;
    while (true) {
      int64_t count1 = 0; // Number of times in a row that first run won
      int64_t count2 = 0; // Number of times in a row that second run won

      /*
        * Do the straightforward thing until (if ever) one run starts
        * winning consistently.
        */
      do {
        if (a[cursor2] < tmp[cursor1]) {
          a[dest++] = a[cursor2++];
          ++count2;
          count1 = 0;
          if (--len2 == 0) goto end;
        } else {
          a[dest++] = tmp[cursor1++];
          ++count1;
          count2 = 0;
          if (--len1 == 1) goto end;
        }
      } while ((count1 | count2) < min_gallop);

      /*
        * One run is winning so consistently that galloping may be a
        * huge win. So try that, and continue galloping until (if ever)
        * neither run appears to be winning consistently anymore.
        */
      do {
        count1 = GallopRight(a[cursor2], tmp, cursor1, len1, 0);
        if (count1 != 0) {
          memcpy(&a[dest], &tmp[cursor1], count1 * sizeof(T));
          dest += count1;
          cursor1 += count1;
          len1 -= count1;
          if (len1 <= 1) goto end;
        }
        a[dest++] = a[cursor2++];
        if (--len2 == 0) goto end;

        count2 = GallopLeft(tmp[cursor1], a, cursor2, len2, 0);
        if (count2 != 0) {
          memcpy(&a[dest], &a[cursor2], count2 * sizeof(T));
          dest += count2;
          cursor2 += count2;
          len2 -= count2;
          if (len2 == 0) goto end;
        }
        a[dest++] = tmp[cursor1++];
        if (--len1 == 1) goto end;
        --min_gallop;
      } while (count1 >= MIN_GALLOP | count2 >= MIN_GALLOP);
      if (min_gallop < 0) min_gallop = 0;
      min_gallop += 2;  // Penalize for leaving gallop mode
    }  // End of "outer" loop

end:
    min_gallop_ = min_gallop < 1 ? 1 : min_gallop;  // Write back to field

    if (len1 == 1) {
      memcpy(&a[dest], &a[cursor2], len2 * sizeof(T));
      a[dest + len2] = tmp[cursor1]; //  Last elt of run 1 to end of merge
    } else {
      memcpy(&a[dest], &tmp[cursor1], len1 * sizeof(T));
    }
  }

  void MergeHi(int64_t base1, int64_t len1, int64_t base2, int64_t len2) {
    CopyToTmp(base2, len2);

    int64_t cursor1 = base1 + len1 - 1;  // Indexes into a
    int64_t cursor2 = len2 - 1;          // Indexes into tmp array
    int64_t dest = base2 + len2 - 1;     // Indexes into a
    T* a = a_;
    T* tmp = &tmp_[0];

    // Move last element of first run and deal with degenerate cases
    a[dest--] = a[cursor1--];
    if (--len1 == 0) {
      memcpy(&a[dest - (len2 - 1)], &tmp[0], len2 * sizeof(T));
      return;
    }

    if (len2 == 1) {
      dest -= len1;
      cursor1 -= len1;
      memcpy(&a[dest + 1], &a[cursor1 + 1], len1 * sizeof(T));
      a[dest] = tmp[cursor2];
      return;
    }

    int64_t min_gallop = min_gallop_;
    while (true) {
      int64_t count1 = 0; // Number of times in a row that first run won
      int64_t count2 = 0; // Number of times in a row that second run won

      /*
        * Do the straightforward thing until (if ever) one run
        * appears to win consistently.
        */
      do {
        if (tmp[cursor2] < a[cursor1]) {
          a[dest--] = a[cursor1--];
          ++count1;
          count2 = 0;
          if (--len1 == 0) goto end;
        } else {
          a[dest--] = tmp[cursor2--];
          ++count2;
          count1 = 0;
          if (--len2 == 1) goto end;
        }
      } while ((count1 | count2) < min_gallop);

      /*
        * One run is winning so consistently that galloping may be a
        * huge win. So try that, and continue galloping until (if ever)
        * neither run appears to be winning consistently anymore.
        */
      do {
        count1 = len1 - GallopRight(tmp[cursor2], a, base1, len1, len1 - 1);
        if (count1 != 0) {
          dest -= count1;
          cursor1 -= count1;
          len1 -= count1;
          memcpy(&a[dest + 1], &a[cursor1 + 1], count1 * sizeof(T));
          if (len1 == 0) goto end;
        }
        a[dest--] = tmp[cursor2--];
        if (--len2 == 1) goto end;

        count2 = len2 - GallopLeft(a[cursor1], tmp, 0, len2, len2 - 1);
        if (count2 != 0) {
          dest -= count2;
          cursor2 -= count2;
          len2 -= count2;
          memcpy(&a[dest + 1], &tmp[cursor2 + 1], count2 * sizeof(T));
          if (len2 <= 1) goto end;
        }
        a[dest--] = a[cursor1--];
        if (--len1 == 0) goto end;
        --min_gallop;
      } while (count1 >= MIN_GALLOP | count2 >= MIN_GALLOP);

      if (min_gallop < 0) min_gallop = 0;
      min_gallop += 2;  // Penalize for leaving gallop mode
    }  // End of "outer" loop

end:
    min_gallop_ = min_gallop < 1 ? 1 : min_gallop;  // Write back to field

    if (len2 == 1) {
      dest -= len1;
      cursor1 -= len1;
      memcpy(&a[dest + 1], &a[cursor1 + 1], len1 * sizeof(T));
      a[dest] = tmp[cursor2];  // Move first elt of run2 to front of merge
    } else {
      memcpy(&a[dest - (len2 - 1)], &tmp[0], len2 * sizeof(T));
    }
  }

  void CopyToTmp(int64_t base, int64_t len) {
    tmp_.resize(len);
    memcpy(&tmp_[0], &a_[base], sizeof(T) * len);
  }
};

void PrintArray(int* a, int n) {
  for (int i = 0; i < n; ++i) {
    printf("%d ", a[i]);
  }
  printf("\n");
}

void PrintArray(double* a, int n) {
  for (int i = 0; i < n; ++i) {
    printf("%f ", a[i]);
  }
  printf("\n");
}

void PrintArray(std::string* a, int n) {
  for (int i = 0; i < n; ++i) {
    printf("%s ", a[i].c_str());
  }
  printf("\n");
}

void Verify(int iter, int n, int r) {
  std::vector<Int> v;
  std::vector<int> tmp;

  for (int i = 0; i < n; ++i) {
    v.push_back(i);
  }

  for (int i = 0; i < iter; ++i) {
    for (int j = 0; j < r; ++j) {
      int x = rand() % n;
      int y = rand() % n;
      int t = v[x].i;
      v[x] = v[y];
      v[y] = t;
    }
    //tmp = v;
    TimSort<Int>::Sort(&v[0], n);
    //sort(v.begin(), v.end());
#if 0
    for (int i = 0; i < n; ++i) {
      if (v[i] != i) {
        printf("Bad\n");
        PrintArray(&tmp[0], n);
        PrintArray(&v[0], n);
        return;
      }
    }
#endif
  }
  printf("%f\n", num_compares / 1000000.0f);
}

int main(int argc, char** argv) {
  Verify(20, 10 * 1000 * 1000, 30 * 1000);
  return 0;
//  int a[] = { 2, 1, 3, 6, 5 };
//  int n = sizeof(a) / sizeof(a[0]);

  int k = 100;
  int n = 40;
  int a[n];
  for (int i = 0; i < n; ++i) {
    a[i] = i;
  }

  for (int i = 0; i < k; ++i) {
    int x = rand() % n;
    int y = rand() % n;
    int t = a[x];
    a[x] = a[y];
    a[y] = t;
  }

  PrintArray(a, n);
  TimSort<int>::Sort(a, n);
  PrintArray(a, n);

  return 0;
}
