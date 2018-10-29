#include <stdio.h>
#include <stdlib.h>

#include <cstdint>
#include <vector>

struct Block {
  uint8_t* ptr;
  size_t len;

  Block(uint8_t* p = nullptr, size_t l = 0) : ptr(p), len(l) {}
};

class Allocator {
 public:
  static constexpr size_t alignment = 0;
  static constexpr size_t GoodSize(size_t);

  virtual Block Allocate(size_t size) = 0;
  virtual Block AllocateAll() = 0;

  virtual bool Owns(const Block&) = 0;

  virtual void Deallocate(const Block&) = 0;
  virtual void DeallocateAll() = 0;
};

class Mallocator : public Allocator {
 public:
  Block Allocate(size_t size) {
    return Block((uint8_t*)malloc(size), size);
  }

  Block AllocateAll() {
    return Block();
  }

  bool Owns(const Block&) {
    return true;
  }

  void Deallocate(const Block& block) {
    if (block.ptr != nullptr) free(block.ptr);
  }

  void DeallocateAll() {
  }
};

class StackAllocator : public Allocator {
 public:
  StackAllocator(Block blk) : block_(blk), ptr_(blk.ptr), end_(ptr_ + blk.len) {
  }

  Block Allocate(size_t size) {
    if (ptr_ + size > end_) return Block();
    Block b(ptr_, size);
    ptr_ += size;
    return b;
  }

  Block AllocateAll() {
    Block b(ptr_, end_ - ptr_);
    ptr_ = end_;
    return b;
  }

  bool Owns(const Block& blk) {
    return blk.ptr >= block_.ptr && block_.ptr < end_;
  }

  void Deallocate(const Block& block) {
    if (block.ptr + block.len == ptr_) {
      ptr_ = block.ptr;
    }
  }

  void DeallocateAll() {
    ptr_ = block_.ptr;
  }

 private:
  const Block block_;
  uint8_t* ptr_;
  uint8_t* end_;
};

class FreeListAllocator : public Allocator {
 public:
  FreeListAllocator(Allocator* parent, size_t min_size, size_t max_size,
      int32_t batch_size, int32_t list_size)
    : parent_(parent), min_size_(min_size), max_size_(max_size),
      batch_size_(batch_size), list_size_(list_size) {
  }

  Block Allocate(size_t size) {
    if (size < min_size_ || size > max_size_) return Block();
    if (list_.empty()) {
      return parent_->Allocate(size);
    }
    Block blk = list_[list_.size() - 1];
    list_.pop_back();
    return blk;
  }

  Block AllocateAll() {
    return Block();
  }

  bool Owns(const Block& blk) {
    if (blk.len < min_size_ || blk.len > max_size_) return false;
    return parent_->Owns(blk);
  }

  void Deallocate(const Block& block) {
  }

  void DeallocateAll() {
  }

 private:
  Allocator* parent_;
  const size_t min_size_;
  const size_t max_size_;
  const int32_t batch_size_;
  const int32_t list_size_;

  std::vector<Block> list_;
};

class BitmapBlockAllocator : public Allocator {
 public:
  BitmapBlockAllocator(Allocator* parent, size_t block_size)
    : parent_(parent), block_size_(block_size) {
  }

  Block Allocate(size_t size) {
    return Block();
  }

  Block AllocateAll() {
    return Block();
  }

  bool Owns(const Block& blk) {
    return false;
  }

  void Deallocate(const Block& block) {
  }

  void DeallocateAll() {
  }

 private:
  Allocator* parent_;
  const size_t block_size_;
};

class Segregator : public Allocator {
 public:
  Segregator(Allocator* a1, Allocator* a2, size_t threshold)
    : a1_(a1), a2_(a2), threshold_(threshold) {
  }

  Block Allocate(size_t size) {
    if (size < threshold_) return a1_->Allocate(size);
    return a2_->Allocate(size);
  }

  Block AllocateAll() {
    return Block();
  }

  bool Owns(const Block& blk) {
    if (blk.len < threshold_) return a1_->Owns(blk);
    return a2_->Owns(blk);
  }

  void Deallocate(const Block& blk) {
    if (blk.len < threshold_) {
      a1_->Deallocate(blk);
    } else {
      a2_->Deallocate(blk);
    }
  }

  void DeallocateAll() {
  }

 private:
  Allocator* a1_;
  Allocator* a2_;
  const size_t threshold_;
};

class FallbackAllocator : public Allocator {
 public:
  FallbackAllocator(Allocator* primary, Allocator* fallback)
    : primary_(primary), fallback_(fallback) {
  }

  Block Allocate(size_t size) {
    Block blk = primary_->Allocate(size);
    if (blk.ptr == nullptr) fallback_->Allocate(size);
    return blk;
  }

  Block AllocateAll() {
    Block blk = primary_->AllocateAll();
    if (blk.ptr == nullptr) fallback_->AllocateAll();
    return blk;
  }

  bool Owns(const Block& blk) {
    return primary_->Owns(blk) || fallback_->Owns(blk);
  }

  void Deallocate(const Block& block) {
    if (primary_->Owns(block)) {
      primary_->Deallocate(block);
    } else {
      fallback_->Deallocate(block);
    }
  }

  void DeallocateAll() {
    primary_->DeallocateAll();
    fallback_->DeallocateAll();
  }

 private:
  Allocator* primary_;
  Allocator* fallback_;
};

int main(int argc, char** argv) {
  Mallocator mallocator;
  StackAllocator stack(mallocator.Allocate(1024));
  FallbackAllocator fallback(&stack, &mallocator);

  mallocator.Deallocate(mallocator.Allocate(100));
  stack.Deallocate(stack.Allocate(100));
  fallback.Deallocate(fallback.Allocate(100));
  fallback.Deallocate(fallback.Allocate(10000));

  printf("Done.\n");
}
