#!/usr/bin/env bash
set -e 
set -u

g++ -g -O3 -msse4.2 -Wall -Wno-sign-compare -fprofile-generate main.cc `llvm-config --cppflags --ldflags --libs core jit native` -o pgo-test
./pgo-test
g++ -g -O3 -msse4.2 -Wall -Wno-sign-compare -fprofile-use main.cc `llvm-config --cppflags --ldflags --libs core jit native` -o pgo-test
./pgo-test
