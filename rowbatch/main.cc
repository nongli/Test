#include <stdio.h>
#include <boost/cstdint.hpp>

class SlotDesc {
 public:
  SlotDesc(int o, int off, int size) {
    ordinal = o;
    byte_offset = off;
    byte_size = size;
  }

  int ordinal;
  int byte_offset;
  int byte_size;
};

class RowBasedBatch {
 public:
  RowBasedBatch(char* data, int row_size, int num_rows) {
    data_ = data;
    num_rows_ = num_rows;
    row_size_ = row_size;
  }

  class Iterator {
   public:
    void Next() {
      data_ += stride_;
    }

    bool End() {
      return data_ == end_;
    }

    void* Value() {
      return data_;
    }

   private:
    friend class RowBasedBatch;
    Iterator(char* start, int stride, int num_rows) {
      data_ = start;
      stride_ = stride;
      end_ = data_ + stride_ * num_rows;
    }
    char* data_, *end_;
    int stride_;
  };

  Iterator Begin(const SlotDesc* desc) {
    return Iterator(data_ + desc->byte_offset, row_size_, num_rows_);
  }

 private:
  char* data_;
  int row_size_;
  int num_rows_;
};

class ColBasedBatch {
 public:
  ColBasedBatch(char* data1, char* data2, char* data3, int num_rows) {
    data_[0] = data1;
    data_[1] = data2;
    data_[2] = data3;
    num_rows_ = num_rows;
  }

  class Iterator {
   public:
    void Next() {
      data_ += stride_;
    }

    bool End() {
      return data_ == end_;
    }

    void* Value() {
      return data_;
    }

   private:
    friend class ColBasedBatch;
    Iterator(char* start, int stride, int num_rows) {
      data_ = start;
      stride_ = stride;
      end_ = data_ + stride_ * num_rows;
    }
    char* data_, *end_;
    int stride_;
  };

  Iterator Begin(const SlotDesc* desc) {
    return Iterator(data_[desc->ordinal], desc->byte_size, num_rows_);
  }

 private:
  char* data_[3];
  int num_rows_;
};

int main(int argc, char** argv) {
  SlotDesc sd1(0, 0, 4);
  SlotDesc sd2(1, 4, 8);
  SlotDesc sd3(2, 12, 16);
  int row_size = sd1.byte_size + sd2.byte_size + sd3.byte_size;

  int dummy = 0;

  int NUM_ROWS = 1024;
  int64_t NUM_ITERS = 1000000;

  RowBasedBatch row_batch(NULL, row_size, NUM_ROWS);
  ColBasedBatch col_batch(NULL, NULL, NULL, NUM_ROWS);
  for (int64_t i = 0; i < NUM_ITERS; ++i) {
#if 0
    RowBasedBatch::Iterator row1_it = row_batch.Begin(&sd1);
    RowBasedBatch::Iterator row2_it = row_batch.Begin(&sd2);
    RowBasedBatch::Iterator row3_it = row_batch.Begin(&sd3);

    while (!row1_it.End()) {
      if (row1_it.Value()) ++dummy;
      if (row2_it.Value()) ++dummy;
      if (row3_it.Value()) ++dummy;
      row1_it.Next();
      row2_it.Next();
      row3_it.Next();
    }
#else
    ColBasedBatch::Iterator row1_it = col_batch.Begin(&sd1);
    ColBasedBatch::Iterator row2_it = col_batch.Begin(&sd2);
    ColBasedBatch::Iterator row3_it = col_batch.Begin(&sd3);

    while (!row1_it.End()) {
      if (row1_it.Value()) ++dummy;
      if (row2_it.Value()) ++dummy;
      if (row3_it.Value()) ++dummy;
      row1_it.Next();
      row2_it.Next();
      row3_it.Next();
    }

#endif
  }

  printf("Count: %d\n", dummy);
}
