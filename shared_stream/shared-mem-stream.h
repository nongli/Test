#ifndef SHARED_MEMORY_STREAM
#define SHARED_MEMORY_STREAM

#include <boost/cstdint.hpp>
#include <string>

// Cross proc single reader, single writer stream.
class SharedMemoryStream {
 public:
  static SharedMemoryStream* Create(const char* path, int64_t capacity);
  static SharedMemoryStream* Open(const char* path);

  void Close();

  // Returns a buffer for writing of size 'len' bytes. Returns NULL if the requested
  // size is too big (larger than capacity). Blocks if the there is not enough bytes
  // available.
  char* GetBufferForWrite(int64_t len);
  // Commits len written bytes available to be read. The writer cannot modify those
  // bytes until another call to GetBufferForWrite().
  void CommitWrite(int64_t len);

  // Returns the next available bytes to be read, blocking until more bytes are
  // available.
  char* Read(int64_t* bytes_read);

  // Indicates 'len' bytes are done being read. Not valid to be read again until the
  // next call to Read()
  void DoneRead(int64_t len);

  const std::string& path() const { return path_; }
  int fd() const { return fd_; }

 private:
  SharedMemoryStream() : fd_(-1), memory_(NULL) {}

  std::string path_;
  int64_t capacity_;
  int fd_;
  bool is_writer_;

  char* memory_;
};

#endif
