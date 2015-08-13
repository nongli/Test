#ifndef SHARED_MEMORY_STREAM
#define SHARED_MEMORY_STREAM

#include <boost/cstdint.hpp>
#include <string>

// Cross proc single reader, single writer stream backed using shared memory.
// Designed for large bulk message-based transfers with zero copies.
// We can use cross proc semaphores to signal or rely on some other IPC mechanism.
class SharedMemoryStream {
 public:
  static SharedMemoryStream* Create(
      const char* path, int64_t capacity_bytes, int max_messages);
  static SharedMemoryStream* Open(const char* path);

  void Close();

  // Returns a buffer for writing of size 'len' bytes. Returns NULL if the requested
  // size is too big (larger than capacity). Blocks if the there is not enough bytes
  // available.
  char* GetBufferForWrite(int64_t len);
  // Commits a message of len bytes available to be read. The writer cannot modify those
  // bytes until another call to GetBufferForWrite().
  bool CommitMessage(int64_t len);

  // Returns the next available bytes to be read, blocking until more bytes are
  // available.
  char* ReadMessage(int64_t* bytes_read);

  // Indicates the previous message from ReadMessage() is no longer used. The memory
  // is not valid anymore.
  void ReadDone();

  const std::string& path() const { return path_; }
  int fd() const { return fd_; }

 private:
  // The first part of the mmapped region is used as a control structure. It contains
  // the Control structure followed by max_messages * sizeof(Message).
  struct Message {
    int64_t len;
    int64_t offset;
  };

  struct Control {
    // Maximum number of messages that can be queued.
    int max_messages;

    // Logical indices for the next write and read messages. The reader only updates
    // write_idx and the writter only updates write index.
    // write_idx >= read_idx and write_idx == read_idx indicates the stream is empty.
    uint64_t write_idx;
    uint64_t read_idx;

    Message messages[0];
  };

  SharedMemoryStream() : fd_(-1), memory_(NULL) {}

  std::string path_;
  int64_t capacity_;
  int max_messages_;
  int fd_;
  bool is_writer_;

  Control* control_;

  // First byte that is usable for client data.
  char* memory_;

  // Writer only state
  char* write_head_;
};

#endif
