GRPC_CPP_PLUGIN_PATH ?= `which grpc_cpp_plugin`

all:
	g++ -std=c++11 -O3 -DNDEBUG -I./gen-cpp main.cc gen-cpp/transfer.pb.cc gen-grpc/transfer.grpc.pb.cc -o benchmark -lgrpc++ -lgpr -lprotobuf -lbenchmark -lboost_system -lboost_thread -lpthread -ldl -lrt

clean:
	rm -f benchmark

protos: transfer.proto
	mkdir -p gen-cpp
	mkdir -p gen-grpc
	protoc transfer.proto --cpp_out=gen-cpp
	protoc transfer.proto --grpc_out=gen-grpc --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_PATH}
