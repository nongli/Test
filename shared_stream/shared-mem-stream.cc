#include "shared-mem-stream.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>


SharedMemoryStream* SharedMemoryStream::Create(const char* path, int64_t capacity) {
  int fd = shm_open(path, O_CREAT | O_RDWR, 0666);
  if (fd < 0) {
    printf("Could not open: %s\n", path);
    return NULL;
  }
  ftruncate(fd, capacity);

  SharedMemoryStream* stream = new SharedMemoryStream;
  stream->path_ = path;
  stream->capacity_ = capacity;
  stream->fd_ = fd;
  stream->is_writer_ = true;
  stream->memory_ = (char*)mmap(0, capacity, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (stream->memory_ == MAP_FAILED) {
    printf("Could not mmap.\n");
    stream->Close();
    return NULL;
  }
  return stream;
}

SharedMemoryStream* SharedMemoryStream::Open(const char* path) {
  int fd = shm_open(path, O_RDONLY, 0666);
  if (fd < 0) {
    printf("Could not open: %s\n", path);
    return NULL;
  }
  struct stat file_stat;
  if (fstat(fd, &file_stat) < 0) {
    printf("Could not get file stat.");
    return NULL;
  }

  SharedMemoryStream* stream = new SharedMemoryStream;
  stream->path_ = path;
  stream->capacity_ = file_stat.st_size;
  stream->fd_ = fd;
  stream->is_writer_ = false;
  stream->memory_ = (char*)mmap(0, stream->capacity_, PROT_READ , MAP_SHARED, fd, 0);
  if (stream->memory_ == MAP_FAILED) {
    printf("Could not mmap.\n");
    stream->Close();
    return NULL;
  }
  return stream;
}

char* SharedMemoryStream::GetBufferForWrite(int64_t len) {
  if (!is_writer_) return NULL;
  if (len > capacity_) return NULL;
  return memory_;
}

char* SharedMemoryStream::Read(int64_t* len) {
  if (is_writer_) return NULL;
  return memory_;
}

void SharedMemoryStream::Close() {
  if (!path_.empty()) shm_unlink(path_.c_str());
  delete this;
}


int main(int argc, char** argv) {
  const int64_t CAPACITY = 4096;
  const char* PATH = "/test";

  SharedMemoryStream* writer = SharedMemoryStream::Create(PATH, CAPACITY);
  if (writer == NULL) {
    printf("Couldn't create writer.\n");
    return -1;
  }
  SharedMemoryStream* reader = SharedMemoryStream::Open(writer->path().c_str());
  if (reader == NULL) {
    printf("Couldn't create reader.\n");
    writer->Close();
    return -1;
  }

  const char* MSG = "this works.";
  char* write_buffer = writer->GetBufferForWrite(1024);
  sprintf(write_buffer, "%s", MSG);


  int64_t read_len;
  char* read_buffer = reader->Read(&read_len);
  printf("READ: %s\n", read_buffer);

  writer->Close();
  reader->Close();


  printf("Done.\n");
  return 0;
}
