all: test.cc
	g++ -g fn.cc -fpic -shared -o fn.so
	g++ -g test.cc `llvm-config --cppflags --ldflags --libs core jit native` -o example
