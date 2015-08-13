#include "shared-mem-stream.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

SharedMemoryStream* SharedMemoryStream::Create(const char* path, int64_t capacity,
    int max_messages) {
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
  char* memory = (char*)mmap(0, capacity, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (memory == MAP_FAILED) {
    printf("Could not mmap.\n");
    stream->Close();
    return NULL;
  }

  stream->control_ = (Control*)memory;
  memset(stream->control_, 0, sizeof(Control));
  stream->control_->max_messages = max_messages;
  stream->memory_ = memory + sizeof(Control) + max_messages * sizeof(Message);
  stream->write_head_ = stream->memory_;
  return stream;
}

SharedMemoryStream* SharedMemoryStream::Open(const char* path) {
  int fd = shm_open(path, O_RDWR, 0666);
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
  char* memory =
      (char*)mmap(0, stream->capacity_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (memory == MAP_FAILED) {
    printf("Could not mmap.\n");
    stream->Close();
    return NULL;
  }
  stream->control_ = (Control*)memory;
  stream->memory_ =
      memory + sizeof(Control) + stream->control_->max_messages * sizeof(Message);
  return stream;
}

char* SharedMemoryStream::GetBufferForWrite(int64_t len) {
  if (!is_writer_) return NULL;
  if (len > capacity_) return NULL;
  while (true) {
    int bytes_used = write_head_ - memory_;
    if (capacity_ - bytes_used >= len) break;
    // wait. When we get signaled by the reader, we can wrap around and
    // reset write_head_ to memory_
  }
  return write_head_;
}

bool SharedMemoryStream::CommitMessage(int64_t len) {
  while (true) {
    uint64_t num_messages = control_->write_idx - control_->read_idx;
    if (num_messages != control_->max_messages) break;
    // wait
  }
  int message_idx = control_->write_idx % control_->max_messages;
  control_->messages[message_idx].offset = write_head_ - memory_;
  control_->messages[message_idx].len = len;
  ++control_->write_idx;
  // Signal
  write_head_ += len;
  return true;
}

char* SharedMemoryStream::ReadMessage(int64_t* len) {
  if (is_writer_) return NULL;

  while (true) {
    uint64_t num_messages = control_->write_idx - control_->read_idx;
    if (num_messages != 0) break;
    // wait
  }

  int message_idx = control_->read_idx % control_->max_messages;
  *len = control_->messages[message_idx].len;
  return memory_ + control_->messages[message_idx].offset;
}

void SharedMemoryStream::ReadDone() {
  int message_idx = control_->read_idx % control_->max_messages;
  ++control_->read_idx;
  // Signal
}

void SharedMemoryStream::Close() {
  if (!path_.empty()) shm_unlink(path_.c_str());
  delete this;
}


int main(int argc, char** argv) {
  const int64_t CAPACITY = 4096;
  const int MAX_MESSAGES = 10;
  const char* PATH = "/test";

  SharedMemoryStream* writer = SharedMemoryStream::Create(PATH, CAPACITY, MAX_MESSAGES);
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
  int bytes_written = sprintf(write_buffer, "%s", MSG);
  writer->CommitMessage(bytes_written);

  int64_t read_len;
  char* read_buffer = reader->ReadMessage(&read_len);
  printf("READ: %s(%ld)\n", read_buffer, read_len);
  reader->ReadDone();

  writer->Close();
  reader->Close();


  printf("Done.\n");
  return 0;
}
