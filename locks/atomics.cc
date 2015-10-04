#include <iostream>
#include <boost/thread/thread.hpp>
#include "boost/interprocess/detail/atomic.hpp"

using namespace boost::interprocess::ipcdetail;

volatile boost::uint32_t value = 0;

void loop(int limit) {
  while (value < limit) {
    atomic_inc32(&value);
  }
}

int main() {
  boost::thread_group threads;
  for (int i = 0; i < 8; ++i) {
    threads.add_thread(new boost::thread(loop, 10000000));
  }
  threads.join_all();
  std::cout << "Spin locks: " << atomic_read32(&value) << std::endl;
}
