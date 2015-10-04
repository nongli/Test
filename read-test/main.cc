// Copyright (c) 2011 Cloudera, Inc. All rights reserved.

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <boost/thread/thread.hpp>

using namespace boost;
using namespace std;

static void ReadAndProcessFile(int fd, int buffer_size, int* result) {
  ssize_t bytes_read = 0;
  int count = 0;
  char* buffer = reinterpret_cast<char*>(malloc(buffer_size));
  while ((bytes_read = read(fd, buffer, buffer_size)) > 0) {
    for (int i = 0; i < bytes_read; ++i) {
      if (buffer[i] == 'a') ++count;
    }
  }
  free(buffer);
  *result = count;
  printf("Thread Done.\n");
}

int main(int argc, char** argv) {
  if (argc != 4) {
    printf("Usage: disk-read-test <dir_path> <num_threads> <read_size (in KB)>\n");
    return -1;
  }

  const char* directory = argv[1];
  int num_threads = atoi(argv[2]);
  int read_size = atoi(argv[3]) * 1024;

  read_size = 32;

  vector<int> file_descriptors;
  vector<int> results;
  file_descriptors.resize(num_threads);
  results.resize(num_threads);

  for (int i = 0; i < num_threads; ++i) {
    stringstream ss;
    ss << directory << (i + 1) << "/data";
    int fd = open(ss.str().c_str(), O_RDONLY, O_DIRECT);
    if (fd == -1) {
      printf("Could not find file: %s\n", ss.str().c_str());
      return 1;
    }
    file_descriptors[i] = fd;
  }

  cout << "Read Size: " << read_size << endl;

  thread_group thread_group;
  for (int i = 0; i < num_threads; ++i) {
    thread_group.add_thread(new thread(
          &ReadAndProcessFile, file_descriptors[i], read_size, &results[i]));
  }
  thread_group.join_all();

  for (int i = 0; i < num_threads; ++i) {
    printf("Result from thread %d: %d\n", i, results[i]);
  }
  
  cout << "Done." << endl;

  return 0;
}
