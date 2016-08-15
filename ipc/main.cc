#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

#include <boost/interprocess/anonymous_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include <benchmark/benchmark.h>
#include <grpc++/grpc++.h>

#include "gen-cpp/transfer.pb.h"
#include "gen-grpc/transfer.grpc.pb.h"
#include "gen-flatbuf/transfer_generated.h"

using boost::asio::ip::tcp;
using namespace boost::interprocess;

const int NUM_COLS = 5;
const int NUM_ROWS = 64 * 1024 * 4;

struct Buffer {
  uint8_t* buffer;
  int len;
};

// Roughly maps to a serialized columnar structure
struct TestData {
  std::vector<Buffer> columns;
  int64_t num_bytes;
  int expected_sum;
  bool owns_memory;


  /**
   * Serialize to and from Protos
   */
  void ToProtoBuf(ipc::TransferData* proto_data) const {
    for (int i = 0; i < columns.size(); ++i) {
      ipc::Buffer* buffer = proto_data->add_columns();
      buffer->set_data((char*)columns[i].buffer, columns[i].len);
    }
  }

  void FromProtoBuf(const ipc::TransferData& proto_data) {
    num_bytes = 0;
    for (int i = 0; i < columns.size(); ++i) {
      columns[i].buffer = (uint8_t*)proto_data.columns(i).data().data();
      columns[i].len = proto_data.columns(i).data().size();
    }
  }

  /**
   * Serialize to and from FlatBufs
   */
  void ToFlatBuf(flatbuffers::FlatBufferBuilder* builder) const {
    std::vector<flatbuffers::Offset<ipc::Column>> serialized_columns;
    for (int i = 0; i < columns.size(); ++i) {
      flatbuffers::Offset<ipc::Column> c = ipc::CreateColumn(
          *builder, builder->CreateVector((int8_t*)columns[i].buffer, columns[i].len));
      serialized_columns.push_back(c);
    }
    builder->Finish(ipc::CreateBatch(*builder, builder->CreateVector(serialized_columns)));
  }

  void FromFlatBuf(int len, const uint8_t* buffer) {
    num_bytes = 0;
    const ipc::Batch* batch = ipc::GetBatch(buffer);
    for (int i = 0; i < columns.size(); ++i) {
      const flatbuffers::Vector<int8_t>* data = batch->columns()->Get(i)->data();
      columns[i].len = data->size();
      columns[i].buffer = (uint8_t*)data->data();
      num_bytes += data->size();
    }
  }

  // Max size of the buffers.
  static int MaxSize() {
    return NUM_ROWS * NUM_COLS;
  }
};

/**
 * Dummy computation
 */
int SumData(const TestData& data) {
  int val = 0;
  for (int i = 0; i < data.columns.size(); i++) {
    const Buffer& col = data.columns[i];
    for (int j = 0; j < col.len; ++j) {
      val += col.buffer[j];
    }
  }
  return val;
}

/**
 * Allocates the columns and optionally the buffers backing the columns.
 */
void AllocateBuffers(TestData* data, bool allocate_buffers) {
  data->owns_memory = allocate_buffers;
  data->columns.resize(NUM_COLS);
  for (int i = 0; i < NUM_COLS; ++i) {
    if (allocate_buffers) {
      data->columns[i].buffer = (uint8_t*)malloc(NUM_ROWS);
      data->columns[i].len = NUM_ROWS;
    } else {
      data->columns[i].buffer = nullptr;
      data->columns[i].len = 0;
    }
  }
}

void FreeBuffers(TestData* data) {
  if (!data->owns_memory) return;
  for (int i = 0; i < data->columns.size(); ++i) {
    if (data->columns[i].buffer != nullptr) {
      free(data->columns[i].buffer);
      data->columns[i].buffer = nullptr;
      data->columns[i].len = 0;
    }
  }
}

void InitData(TestData* data) {
  data->num_bytes = 0;
  for (int i = 0; i < data->columns.size(); ++i) {
    for (int j = 0; j < NUM_ROWS; ++j) {
      data->columns[i].buffer[j] = i * j;
    }
    data->num_bytes += NUM_ROWS;
  }
  data->expected_sum = SumData(*data);
}

/**
 * Base class for different ways to send data.
 */
class Transport {
 public:
  virtual ~Transport() {}
  virtual const TestData* Transfer(const TestData& data) = 0;
};

// No op, doesn't move the bytes.
class NoopTransport : public Transport {
 public:
  const TestData* Transfer(const TestData& data) { return &data; }
};

// Simple memcpy of the bytes.
class MemcpyTransport : public Transport {
 public:
  MemcpyTransport() {
    AllocateBuffers(&data, true);
  }
  ~MemcpyTransport() {
    FreeBuffers(&data);
  }

  const TestData* Transfer(const TestData& src) {
    for (int i = 0; i < src.columns.size(); ++i) {
      memcpy(data.columns[i].buffer, src.columns[i].buffer, src.columns[i].len);
    }
    data.num_bytes = src.num_bytes;
    return &data;
  }

 private:
  TestData data;
};

// Writes and reads bytes from tmp file.
// TODO: this test is not very useful, a real use case would do this
// multiple batches at a time.
class TmpFileTransport : public Transport {
 public:
  TmpFileTransport(const char* path) {
    AllocateBuffers(&data, true);
    if (path == nullptr) {
      tmpfile = std::tmpfile();
    } else {
      tmpfile = fopen(path, "w+");
    }
    if (tmpfile == nullptr) {
      std::cerr << "Could not create local file." << std::endl;
    }
  }

  ~TmpFileTransport() {
    FreeBuffers(&data);
    fclose(tmpfile);
  }

  const TestData* Transfer(const TestData& src) {
    fseek(tmpfile, 0, SEEK_SET);
    // For each column, write len followed by bytes.
    for (int i = 0; i < src.columns.size(); ++i) {
      fwrite(&src.columns[i].len, sizeof(int), 1, tmpfile);
      fwrite(src.columns[i].buffer, src.columns[i].len, 1, tmpfile);
    }
    fflush(tmpfile);

    // Read it back.
    data.num_bytes = 0;
    fseek(tmpfile, 0, SEEK_SET);
    for (int i = 0; i < data.columns.size(); ++i) {
      int len;
      int read = fread(&len, sizeof(len), 1, tmpfile);
      read = fread(data.columns[i].buffer, len, 1, tmpfile);
      data.num_bytes += len;
    }
    return &data;
  }

 private:
  std::FILE* tmpfile;
  TestData data;
};

struct Serialization {
  enum Type {
    CUSTOM,
    PROTOS,
    FLAT_BUF,
  };
};

/**
 * Server thread that will send send when it receives the '0' control message.
 */
void TcpServerThread(const TestData** data, Serialization::Type serialization) {
  boost::asio::io_service io_service;
  tcp::acceptor server(io_service, tcp::endpoint(tcp::v4(), 1234));
  tcp::socket socket(io_service);
  server.accept(socket);

  while (true) {
    int msg;
    boost::asio::read(socket, boost::asio::buffer(&msg, sizeof(msg)));
    if (msg == -1) break;

    switch (serialization) {
      case Serialization::PROTOS: {
        ipc::TransferData proto_data;
        (*data)->ToProtoBuf(&proto_data);
        std::string serialized;
        proto_data.SerializeToString(&serialized);
        int serialized_len = serialized.size();
        boost::asio::write(socket, boost::asio::buffer(&serialized_len, sizeof(int)));
        boost::asio::write(socket, boost::asio::buffer(serialized.data(), serialized_len));
        break;
      }
      case Serialization::CUSTOM: {
        for (int i = 0; i < (*data)->columns.size(); ++i) {
          Buffer column = (*data)->columns[i];
          boost::asio::write(socket, boost::asio::buffer(&column.len, sizeof(int)));
          boost::asio::write(socket, boost::asio::buffer(column.buffer, column.len));
        }
        break;
      }
      case Serialization::FLAT_BUF: {
        flatbuffers::FlatBufferBuilder builder;
        (*data)->ToFlatBuf(&builder);
        int serialized_len = builder.GetSize();
        boost::asio::write(socket, boost::asio::buffer(&serialized_len, sizeof(int)));
        boost::asio::write(socket, boost::asio::buffer(
            builder.GetBufferPointer(), serialized_len));
        break;
      }
    }
  }
}

// Reads and writes the data over tcp to localhost.
class TcpTransport : public Transport {
 public:
  TcpTransport(Serialization::Type serialization)
    : serialization(serialization),
      send_data(nullptr),
      server_thread(boost::bind(TcpServerThread, &send_data, serialization)) {
    switch (serialization) {
      case Serialization::FLAT_BUF:
      case Serialization::PROTOS: {
        // If protos, the result just points into the deserialized struct to avoid a copy.
        AllocateBuffers(&result_data, false);
        break;
      }
      case Serialization::CUSTOM: {
        AllocateBuffers(&result_data, true);
        break;
      }
    }

    tcp::resolver resolver(io_service);
    tcp::resolver::query query(tcp::v4(), "localhost", "1234");
    tcp::resolver::iterator iterator = resolver.resolve(query);
    socket.reset(new tcp::socket(io_service));
    boost::asio::connect(*socket, iterator);
  }

  ~TcpTransport() {
    // Send -1 to the server, which gets it to start sending data.
    int msg = -1;
    boost::asio::write(*socket, boost::asio::buffer(&msg, sizeof(msg)));
    server_thread.join();
    FreeBuffers(&result_data);
  }

  const TestData* Transfer(const TestData& src) {
    send_data = &src;
    // Send 0 to the server, which gets it to start sending data.
    int msg = 0;
    boost::asio::write(*socket, boost::asio::buffer(&msg, sizeof(msg)));

    // Read the bytes.
    boost::system::error_code error;
    switch (serialization) {
      case Serialization::PROTOS: {
        int serialized_len;
        socket->read_some(boost::asio::buffer(&serialized_len, sizeof(int)), error);
        std::string serialized;
        serialized.resize(serialized_len);
        ReadFully((uint8_t*)serialized.data(), serialized_len);

        proto_data.Clear();
        proto_data.ParseFromString(serialized);
        result_data.FromProtoBuf(proto_data);
        break;
      }
      case Serialization::CUSTOM: {
        result_data.num_bytes = 0;
        for (int i = 0; i < result_data.columns.size(); ++i) {
          Buffer* column = &result_data.columns[i];
          socket->read_some(boost::asio::buffer(&column->len, sizeof(int)), error);
          if (error) throw boost::system::system_error(error);
          ReadFully(column->buffer, column->len);
          if (error) throw boost::system::system_error(error);
          result_data.num_bytes += column->len;
        }
        break;
      }
      case Serialization::FLAT_BUF: {
        int serialized_len;
        socket->read_some(boost::asio::buffer(&serialized_len, sizeof(int)), error);
        serialized_buffer.resize(serialized_len);
        ReadFully(serialized_buffer.data(), serialized_len);
        result_data.FromFlatBuf(serialized_len, serialized_buffer.data());
        break;
      }
    }
    return &result_data;
  }

 private:
  const Serialization::Type serialization;
  boost::asio::io_service io_service;
  const TestData* send_data;
  boost::thread server_thread;
  boost::shared_ptr<tcp::socket> socket;

  ipc::TransferData proto_data;
  TestData result_data;
  std::vector<uint8_t> serialized_buffer;

  void ReadFully(uint8_t* buffer, int len) {
    boost::system::error_code error;
    int total_read = 0;
    while (total_read < len) {
      int left = len - total_read;
      size_t len = socket->read_some(
          boost::asio::buffer(buffer + total_read, left), error);
      total_read += len;
    }
  }
};

const char* SHARED_MEM_NAME = "IPC_TEST_SHARED_MEM";

class TransferServiceImpl final : public ipc::TransferService::Service {
 public:
  TransferServiceImpl(const TestData** data, bool shared_mem)
    : data(data),
      shared_mem(shared_mem) {
    if (shared_mem) {
      shared_memory_object::remove(SHARED_MEM_NAME);
      shared_memory_object shm;
      shm = shared_memory_object(create_only, SHARED_MEM_NAME, read_write);
      shm.truncate(TestData::MaxSize());
      region = mapped_region(shm, read_write);
    }
  }

  virtual grpc::Status Transfer(grpc::ServerContext* context,
      const ipc::Empty* request, ::ipc::TransferData* response) {
    (*data)->ToProtoBuf(response);
    return grpc::Status::OK;
  }

  virtual grpc::Status TransferControl(grpc::ServerContext* context,
      const ipc::Empty* request, ::ipc::Empty* response) {
    if (!shared_mem) {
      std::cerr << "TransferControl() only valid with shared memory." << std::endl;
    } else {
      char* dst = (char*)region.get_address();
      for (int i = 0; i < (*data)->columns.size(); ++i) {
        memcpy(dst, (*data)->columns[i].buffer, NUM_ROWS);
        dst += NUM_ROWS;
      }
    }
    return grpc::Status::OK;
  }

  ~TransferServiceImpl() {}

 private:
  const TestData** data;
  const bool shared_mem;

  // Shared memory region.
  mapped_region region;
};

void GrpcServerThread(grpc::Server* server) {
  server->Wait();
}

class GrpcTransport : public Transport {
 public:
  GrpcTransport(bool shared_mem)
    : shared_mem(shared_mem),
      send_data(nullptr),
      service(&send_data, shared_mem),
      client(ipc::TransferService::NewStub(
          grpc::CreateChannel(
              "localhost:50051", grpc::InsecureChannelCredentials()))) {
    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    server = std::move(builder.BuildAndStart());
    server_thread.reset(
        new boost::thread(boost::bind(GrpcServerThread, server.get())));

    // The result just points into the proto struct to avoid a copy.
    result_data.columns.resize(NUM_COLS);
    result_data.owns_memory = false;

    if (shared_mem) {
      shared_memory_object shm =
          shared_memory_object(open_only, SHARED_MEM_NAME, read_write);
      region = mapped_region(shm, read_write);
    }
  }

  ~GrpcTransport() {
    server->Shutdown();
    server_thread->join();
    if (shared_mem) shared_memory_object::remove(SHARED_MEM_NAME);
  }

  const TestData* Transfer(const TestData& src) {
    send_data = &src;
    ipc::Empty request;
    grpc::ClientContext ctx;
    proto_data.Clear();
    grpc::Status status;

    if (shared_mem) {
      status = client->TransferControl(&ctx, request, &request);
    } else {
      status = client->Transfer(&ctx, request, &proto_data);
    }
    if (!status.ok()) {
      std::cerr << "GRPC failed." << std::endl;
      return nullptr;
    }
    if (shared_mem) {
      result_data.num_bytes = 0;
      // This mapping might be avoidable as well.
      uint8_t* src = (uint8_t*)region.get_address();
      for (int i = 0; i < result_data.columns.size(); ++i) {
        int len = NUM_ROWS;
        result_data.columns[i].buffer = src;
        result_data.columns[i].len = len;
        result_data.num_bytes += len;
        src += len;
      }
    } else {
      result_data.FromProtoBuf(proto_data);
    }
    return &result_data;
  }

 private:
  const bool shared_mem;
  const TestData* send_data;
  TransferServiceImpl service;
  std::unique_ptr<grpc::Server> server;
  std::unique_ptr<boost::thread> server_thread;
  std::unique_ptr<ipc::TransferService::Stub> client;
  ipc::TransferData proto_data;
  TestData result_data;

  // Shared memory region.
  mapped_region region;
};

/**
 * Benchmark code below
 */
void Benchmark(benchmark::State& state, TestData* data, Transport* transport) {
  InitData(data);
  while (state.KeepRunning()) {
    int v = SumData(*transport->Transfer(*data));
    benchmark::DoNotOptimize(&v);
    if (v != data->expected_sum) {
      std::cerr << "Sum was incorrect." << std::endl;
      return;
    }
  }
  state.SetBytesProcessed(data->num_bytes * state.iterations());
  state.SetItemsProcessed(NUM_ROWS * state.iterations());
}

// Utility to benchmark non-shared memory cases.
void Benchmark(benchmark::State& state, Transport* transport) {
  TestData data;
  AllocateBuffers(&data, true);
  Benchmark(state, &data, transport);
  FreeBuffers(&data);
}

/**
 * Baseline that does not move the data
 */
static void BM_Local(benchmark::State& state) {
  NoopTransport transport;
  Benchmark(state, &transport);
}

/**
 * Benchmark using memcpy files.
 */
static void BM_Copy(benchmark::State& state) {
  MemcpyTransport transport;
  Benchmark(state, &transport);
}

/**
 * Benchmark using tmp files.
 */
static void BM_TmpFile(benchmark::State& state) {
  TmpFileTransport transport(nullptr);
  Benchmark(state, &transport);
}

/**
 * Benchmark with named file.
 */
static void BM_LocalFile(benchmark::State& state) {
  TmpFileTransport transport("/tmp/ipc-benchmark");
  Benchmark(state, &transport);
}

/**
 * Benchmark using tcp socket.
 */
static void BM_Tcp(benchmark::State& state) {
  TcpTransport transport(Serialization::CUSTOM);
  Benchmark(state, &transport);
}

/**
 * Benchmark using protos over tcp socket.
 */
static void BM_TcpProtos(benchmark::State& state) {
  TcpTransport transport(Serialization::PROTOS);
  Benchmark(state, &transport);
}

/**
 * Benchmark using flatbuf over tcp socket.
 */
static void BM_TcpFlatBuf(benchmark::State& state) {
  TcpTransport transport(Serialization::FLAT_BUF);
  Benchmark(state, &transport);
}

/**
 * Benchmark using GRPC (data sent via protos).
 */
static void BM_Grpc(benchmark::State& state) {
  GrpcTransport transport(false);
  Benchmark(state, &transport);
}

/**
 * Benchmark sending data via shared mem, control handled via grpc
 * In this version, the data is serialized on the writer side to shared
 * memory and no deserialization on the reader.
 */
static void BM_GrpcSharedMem(benchmark::State& state) {
  GrpcTransport transport(true);
  Benchmark(state, &transport);
}

BENCHMARK(BM_Local);
BENCHMARK(BM_Copy);
BENCHMARK(BM_TmpFile);
BENCHMARK(BM_LocalFile);
BENCHMARK(BM_Tcp);
BENCHMARK(BM_TcpProtos);
BENCHMARK(BM_TcpFlatBuf);
BENCHMARK(BM_Grpc);
BENCHMARK(BM_GrpcSharedMem);

BENCHMARK_MAIN();
