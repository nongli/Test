#include <math.h>
#include <stdio.h>
#include <time.h>
#include <boost/cstdint.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/types.h>

using namespace std;

struct Event {
  int32_t id;
  int32_t thread_id;
  int64_t time_ns;
  int64_t args[2];
};

struct EventDesc {
  int32_t id;
  int num_args;
  const char* name;
  const char* desc;
};

struct EventId {
  enum Type {
    UNKNOWN,
    FOO,
    BAR
  };
};

EventDesc event_descs[] = {
  { EventId::UNKNOWN, 0, "Unknown", "Unknown" },
  { EventId::FOO, 0, "Foo", "" },
  { EventId::BAR, 1, "Bar", "(%d)" }
};

#define MAX_EVENTS 1024

Event events[MAX_EVENTS];
int num_events = 0;

inline uint64_t CurrentTimeNs() {
  timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec * 1000L * 1000L * 1000L + t.tv_nsec;
}

#define TRACE_START(e)\
  events[num_events].id = e;\
  events[num_events].thread_id = 0;\
  events[num_events].time_ns = CurrentTimeNs();\
  ++num_events;

#define TRACE_START1(e, arg0)\
  events[num_events].id = e;\
  events[num_events].thread_id = 0;\
  events[num_events].time_ns = CurrentTimeNs();\
  events[num_events].args[0] = arg0;\
  ++num_events;

#define TRACE_START2(e, arg0, arg1)\
  events[num_events].id = e;\
  events[num_events].thread_id = 0;\
  events[num_events].time_ns = CurrentTimeNs();\
  events[num_events].args[0] = arg0;\
  events[num_events].args[1] = arg1;\
  ++num_events;

#define TRACE_END(e)\
  events[num_events].id = -e;\
  events[num_events].thread_id = 0;\
  events[num_events].time_ns = CurrentTimeNs();\
  ++num_events;

void DumpEvents() {
  if (num_events == 0) return;
  uint64_t start_time = events[0].time_ns;

  int indent = -1;
  for (int i = 0; i < num_events; ++i) {
    const Event& e = events[i];
    int32_t event_id = abs(e.id);
    bool is_start = e.id > 0;
    const EventDesc& desc = event_descs[event_id];

    char buffer[1024];
    switch (desc.num_args) {
      case 0:
        snprintf(buffer, 1024, desc.desc);
        break;
      case 1:
        snprintf(buffer, 1024, desc.desc, e.args[0]);
        break;
      case 2:
        snprintf(buffer, 1024, desc.desc, e.args[0], e.args[1]);
        break;
    }

    if (is_start) ++indent;
    stringstream ss;
    for (int i = 0; i < indent; ++i) {
      ss << "  ";
    }
    ss << (is_start ? "Start " : "End ")
       << "(" << (e.time_ns - start_time) << " tid=" << e.thread_id << "): "
       << desc.name << buffer;
    cout << ss.str() << endl;

    if (!is_start) --indent;
  }
}

string ToChromeJson() {
  stringstream ss;
  ss << "{ \"traceEvents\": [";
  for (int i = 0; i < num_events; ++i) {
    const Event& e = events[i];
    int32_t event_id = abs(e.id);
    bool is_start = e.id > 0;
    const EventDesc& desc = event_descs[event_id];

    ss << "{"
       << "\"cat\": \"Impala\","
       << "\"pid\": " << getpid() << ","
       << "\"tid\": " << e.thread_id << ","
       << "\"ts\": " << e.time_ns / 1000.0 << ","
       << "\"ph\": \"" << (is_start ? "B" : "E") << "\","
       << "\"name\": \"" << desc.name << "\","
       << "\"args\": \"None\""
       << "}";
    if (i != num_events - 1) {
      ss << ",";
    }
    ss << endl;
  }
  ss << "] }";
  return ss.str();
}

void Bar(int a) {
  TRACE_START1(EventId::BAR, a);
  sleep(1);
  TRACE_END(EventId::BAR);
}

void Foo() {
  TRACE_START(EventId::FOO);
  Bar(100);
  TRACE_END(EventId::FOO);
}

int main(int argc, char** argv) {
  Foo();
  Bar(1);
  DumpEvents();

  string json = ToChromeJson();
  cout << json << endl;
  printf("Done.\n");
}
