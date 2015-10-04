#!/usr/bin/env bash

bin_dir=~/Projects/llvm-3.0.src/Release/bin
clang=$bin_dir/clang++
llvm_dis=$bin_dir/llvm-dis
llvm_config=$bin_dir/llvm-config

$clang -emit-llvm -c -O3 loop.cc -o loop.ll
$llvm_dis < ./loop.ll

g++ -g main.cc `$llvm_config --cppflags --ldflags --libs core jit native support bitreader` -o out
