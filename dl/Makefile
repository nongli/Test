all:
	g++ -fPIC -shared library.cc -o test.so
	g++ -ldl -rdynamic main.cc -o out
