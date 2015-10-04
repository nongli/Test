#include <iostream>
#include <boost/thread/thread.hpp>

boost::mutex mutex;
volatile int value = 0;

void loop(int limit) {
  while (value < limit) {
    boost::lock_guard<boost::mutex> guard(mutex);
    ++value;
  }
}

int main() {
  boost::thread_group threads;
  for (int i = 0; i < 8; ++i) {
    threads.add_thread(new boost::thread(loop, 10000000));
  }
  threads.join_all();
  std::cout << "Mutex: " << value << std::endl;
}
