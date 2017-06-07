#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <list>
#include <map>
#include <queue>
#include <random>
#include <set>
#include <vector>

class Cache {
  public:
    virtual ~Cache() {}

    // Returns if the key is in the cache.
    virtual bool Get(int key) = 0;

    // Puts the value in the cache, evicting existing values if necessary
    virtual void Put(int key) = 0;

    // Clears everything
    virtual void Clear() = 0;

  protected:
    Cache(int capacity) : capacity(capacity) { }
    const int capacity;
};

class NullCache : public Cache {
  public:
    explicit NullCache(int capacity) : Cache(capacity) {}
    virtual ~NullCache() {}

    virtual bool Get(int key) { return false; }
    virtual void Put(int key) {}
    virtual void Clear() {}
};

class NoEvictCache : public Cache {
  public:
    explicit NoEvictCache(int capacity) : Cache(capacity) {}
    virtual ~NoEvictCache() {}

    virtual bool Get(int key) { return cache.find(key) != cache.end(); }
    virtual void Put(int key) {
      if (cache.size() < capacity) {
        cache.insert(key);
      }
    }
    virtual void Clear() { cache.clear(); }
  private:
    std::set<int> cache;
};

class LruCache : public Cache {
  public:
    explicit LruCache(int capacity) : Cache(capacity) {}
    virtual ~LruCache() {}

    virtual bool Get(int key) {
      std::map<int, std::list<int>::iterator>::iterator it = cache.find(key);
      if (it == cache.end()) return false;
      history.erase(it->second);
      history.push_front(key);
      cache[key] = history.begin();
      return true;
    }
    virtual void Put(int key) {
      if (cache.size() >= capacity) {
        cache.erase(history.back());
        history.pop_back();
      }
      history.push_front(key);
      cache[key] = history.begin();
    }
    virtual void Clear() {
      cache.clear();
      history.clear();
    }
  private:
    std::map<int, std::list<int>::iterator> cache;
    // Front of least is last accessed, remove from back
    std::list<int> history;
};

class Lru2Cache : public Cache {
  public:
    explicit Lru2Cache(int capacity)
      : Cache(capacity), timestamp(0) {
      }
    virtual ~Lru2Cache() {}

    virtual bool Get(int key) {
      std::map<int, Entry*>::iterator it = cache.find(key);
      if (it == cache.end()) return false;
      ++timestamp;
      it->second->t1 = it->second->t2;
      it->second->t2 = timestamp;
      return true;
    }

    virtual void Put(int key) {
      ++timestamp;
      if (cache.size() < capacity) {
        // Just fits.
        Entry* entry = new Entry();
        entry->key = key;
        entry->t1 = 0;
        entry->t2 = timestamp;
        cache[key] = entry;
        history.push_back(entry);
        return;
      }
      // evict by sorting
      sort(history.begin(), history.end(), EntryLessThan());
      // Remove first entry
      cache.erase(history[0]->key);

      // Repurpose the entry object and add it
      history[0]->key = key;
      history[0]->t1 = 0;
      history[0]->t2 = timestamp;
      cache[key] = history[0];
    }
    virtual void Clear() {
      timestamp = 0;
      cache.clear();
      for (int i = 0; i < history.size(); i++) {
        delete history[i];
      }
      history.clear();
    }
  private:
    struct Entry {
      int key;
      // Last two access times. t1 <= t2
      int64_t t1, t2;
    };

    struct EntryLessThan {
      inline bool operator() (const Entry* e1, const Entry* e2) {
        return e1->t1 < e2->t1;
      }
    };

    std::map<int, Entry*> cache;
    // Entries that are in the cache. This is sorted to get the LRU entry.
    // This should be a priority queue or some data structure like that.
    std::vector<Entry*> history;
    int64_t timestamp;
};

const int CAPACITY = 1000;
std::vector<int> cyclic;
std::vector<int> clumpy;
std::vector<int> normal;
std::vector<int> temporal;
std::vector<int> clumpy_with_scan;

void init() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::normal_distribution<> nd(0, CAPACITY);
  std::uniform_real_distribution<double> ud(0.0, 1.0);

  int temporal_min = 0;
  int temporal_max = CAPACITY;

  for (int i = 0; i < CAPACITY * 10; i++) {
    cyclic.push_back(i % (CAPACITY * 2));
    clumpy.push_back(rand() % (int)(CAPACITY * 1.2));
    normal.push_back((int)nd(gen));

    float r = ud(gen);
    temporal.push_back(r * (temporal_max - temporal_min) + temporal_min);
    temporal_min++;
    temporal_max++;
  }

  for (int i = 0; i < CAPACITY * 4; i++) {
    clumpy_with_scan.push_back(rand() % (int)(CAPACITY * 1.2));
  }
  for (int i = 0; i < CAPACITY * 2; i++) {
    clumpy_with_scan.push_back(CAPACITY * 10 + i);
  }
  for (int i = 0; i < CAPACITY * 4; i++) {
    clumpy_with_scan.push_back(rand() % (int)(CAPACITY * 1.2));
  }
}

void Test(Cache* cache, const std::vector<int>& input) {
  cache->Clear();
  int hits = 0;
  for (int i = 0; i < input.size(); i++) {
    if (cache->Get(input[i])) {
      ++hits;
    } else {
      cache->Put(input[i]);
    }
  }
  float ratio = ((float)hits) / input.size();
  printf("Hits %d/%lu (%.2f)\n", hits, input.size(), ratio);
}

void Test(const char* name, Cache* cache) {
  printf("Running test: %s\n", name);
  printf("  cyclic: ");
  Test(cache, cyclic);
  printf("  clumpy: ");
  Test(cache, clumpy);
  printf("  normal: ");
  Test(cache, normal);
  printf("  temporal: ");
  Test(cache, temporal);
  printf("  clumpy with scan: ");
  Test(cache, clumpy_with_scan);

  delete cache;
}

int main(int argc, char** argv) {
  init();
  // Test("Null Cache", new NullCache(CAPACITY));
  Test("No Evict Cache", new NoEvictCache(CAPACITY));
  Test("LRU Cache", new LruCache(CAPACITY));
  Test("LRU-2 Cache", new Lru2Cache(CAPACITY));
  printf("Done.\n");
  return 0;
}
