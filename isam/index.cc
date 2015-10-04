
// The data for an index page. The node contains entries which are fixed size key
// value pairs. The entries are sorted by key. The key is an index into the values if
// it is a value index. The value is the page offset and len for the next page to go to.
// The next page could be an index page or a data page.

struct IndexEntry {
  int32_t value_byte_offset;
  int32_t page_offset;
  int32_t page_len;
};

// The on disk layout is exactly this object.
// e.g.
//   char* page_buffer = read();
//   Index* index = (Index*)page_buffer;
template <typename T>
class Index {
 public:
  // Returns the Entry that contains 'target'
  // If NULL, it indicates target does not exist. If non-null, the target can
  // be found by reading the next page from IndexEntry.
  const IndexEntry* Index::GetPageNode(const T& target) {
    IndexEntry* prev = NULL;
    // This is illustrative and should be a binary search instead.
    // Each entry is fixed size making binary search possible.
    for (int i = 0; i < num_entries_; ++i) {
      const IndexEntry& entry = entries_[i];
      if (*(T*)(values_ + entry.value_byte_offset) >= target) {
        break;
      }
      prev = &entry;
    }
    return prev;
  }

 private:
  // An array of index entries.
  int num_entries_;
  IndexEntry* entries_;

  // An array of values. values_ + entries_[i].value_byte_offset] is the
  // key value for entry i. The values are plain encoded.
  // TODO: this could be encoded with something else at long as it supports random
  // access.
  uint8_t* values_;
};

