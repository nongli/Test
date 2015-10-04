#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <list>
#define _XOPEN_SOURCE 500
#include <ucontext.h>

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

struct WorkerState {
  int id;
  bool ready;
  ucontext_t schedule_context;
  ucontext_t work_context;

  WorkerState() {
    ready = false;
    memset(&schedule_context, 0, sizeof(ucontext_t));
    memset(&work_context, 0, sizeof(ucontext_t));
  }
};

struct SchedulerState {
  mutex lock;
  condition_variable cv;
  list<int> output_queue;

  vector<WorkerState*> workers;
};

volatile bool done;

void SchedulerThread(SchedulerState* state) {
  while (true) {
    unique_lock<mutex> l(state->lock);
    while (state->output_queue.empty() && !done) {
      state->cv.wait(l);
    }
    if (done) break;

    int worker = state->output_queue.front();
    state->output_queue.pop_front();

    // TODO: switch to Work context
    WorkerState* worker_state = state->workers[worker];
    if (!getcontext(&worker_state->schedule_context)) {
      printf("getcontext failed()");
      return;
    }
  }
}

void Work(WorkerState* state) {
  for (int i = 0; i < 100; ++i) {
    if (state->ready) {
      state->ready = false;
      printf("Output: %d: %d\n", state->id, i);
    } else {
      // Switch to the scheduler context
    }
  }
}

int main(int argc, char** argv) {
  int NUM_WORKERS = 5;

  ucontext_t context;
  getcontext(&context);
  puts("Hello world");
  sleep(1);
  setcontext(&context);

  done = false;
  thread_group worker_threads;

  Worker workers[NUM_WORKERS];
  for (int i = 0; i < NUM_WORKERS; ++i) {
    workers[i].id = i;
    //worker_threads.add_thread(new thread(WorkerThread, &workers[i]));
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
