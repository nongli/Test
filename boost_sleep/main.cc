#include <stdio.h>
#include <boost/thread/thread.hpp>

using namespace boost;
using namespace boost::posix_time;

int main(int argc, char** argv) {
  time_duration duration = seconds(3);
  while (1) {
    this_thread::sleep(duration);
    printf("Boo\n");
  }
  return 0;
}
