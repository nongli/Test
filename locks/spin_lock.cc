#include <iostream>
#include <boost/thread/thread.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>

boost::detail::spinlock lock;
volatile int value = 0;

void loop(int limit) {
  while (value < limit) {
    boost::lock_guard<boost::detail::spinlock> guard(lock);
    ++value;
  }
}

int main() {
  boost::thread_group threads;
  for (int i = 0; i < 8; ++i) {
    threads.add_thread(new boost::thread(loop, 10000000));
  }
  threads.join_all();
  std::cout << "Spin lock: " << value << std::endl;
}
