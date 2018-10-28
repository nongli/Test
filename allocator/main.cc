#include <stdio.h>
#include <stdlib.h>
#include <cstdint>

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

  virtual bool Expand(Block*, size_t delta) = 0;
  virtual void Reallocate(Block*, size_t size) = 0;

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

  bool Expand(Block*, size_t delta) {
    return false;
  }

  void Reallocate(Block* block, size_t size) {
    block->ptr = (uint8_t*)realloc(block->ptr, size);
    block->len = size;
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

  bool Expand(Block* blk, size_t delta) {
    if (blk->ptr + blk->len == ptr_ && ptr_ + delta < end_) {
      ptr_ += delta;
      blk->len += delta;
      return true;
    }
    return false;
  }

  void Reallocate(Block* blk, size_t size) {
    if (blk->ptr + blk->len == ptr_ && blk->ptr + size < end_) {
      // Expand the current one
      ptr_ += size - blk->len;
      blk->len = size;
    } else {
      *blk = Allocate(size);
    }
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

  bool Expand(Block* blk, size_t delta) {
    if (primary_->Owns(*blk)) return primary_->Expand(blk, delta);
    return fallback_->Expand(blk, delta);
  }

  void Reallocate(Block* blk, size_t size) {
    if (primary_->Owns(*blk)) {
      primary_->Reallocate(blk, size);
      if (blk->ptr == nullptr) {
        *blk = fallback_->Allocate(size);
      }
    } else {
      fallback_->Reallocate(blk, size);
    }
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
