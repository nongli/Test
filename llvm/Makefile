all: example.cc
	g++ -g example.cc `llvm-config --cppflags --ldflags --libs core jit native` -o example
