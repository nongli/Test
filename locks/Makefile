all:
	g++ -I/opt/local/include -L/opt/local/lib -lboost_thread-mt -lboost_system-mt -O3 -o no_locks no_locks.cc
	g++ -I/opt/local/include -L/opt/local/lib -lboost_thread-mt -lboost_system-mt -O3 -o atomics atomics.cc
	g++ -I/opt/local/include -L/opt/local/lib -lboost_thread-mt -lboost_system-mt -O3 -o spin_lock spin_lock.cc
	g++ -I/opt/local/include -L/opt/local/lib -lboost_thread-mt -lboost_system-mt -O3 -o mutex mutex.cc
