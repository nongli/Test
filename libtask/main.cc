#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <boost/thread/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>

using namespace boost;
using namespace std;

struct Worker {
  int id;
  mutex lock;
  condition_variable cv;
  bool output;

  Worker() {
    output = false;
  }
};

volatile bool done;

void WorkerThread(Worker* worker) {
  int count = 0;
  
  while (true) {
    unique_lock<mutex> l(worker->lock);
    while (!worker->output && !done) {
      worker->cv.wait(l);
    }
    if (done) break;

    int r = rand() % 2000 + 1000;
    usleep(r * 1000);
 
    ++count;
    printf("Output: %d: %d\n", worker->id, count);
    worker->output = false;
  }

  printf("Thread %d is done.\n", worker->id);
}

int main(int argc, char** argv) {
  int NUM_WORKERS = 5;

  done = false;
  thread_group worker_threads;

  Worker workers[NUM_WORKERS];
  for (int i = 0; i < NUM_WORKERS; ++i) {
    workers[i].id = i;
    worker_threads.add_thread(new thread(WorkerThread, &workers[i]));
  }

  while (true) {
    printf("Input? ");
    char c = getchar();
    getchar();
    if (c == '.') break;
    
    int i = c - '0';
    if (i >= 0 && i < NUM_WORKERS) {
      Worker* worker = &workers[i];
      {
        unique_lock<mutex> l(worker->lock);
        worker->output = true;
      }
      workers[i].cv.notify_one();
    }
  }

  done = true;
  for (int i = 0; i < NUM_WORKERS; ++i) {
    unique_lock<mutex> l(workers[i].lock);
    workers[i].cv.notify_one();
  }

  worker_threads.join_all();
  
  printf("Done.\n");
  return 0;
}
