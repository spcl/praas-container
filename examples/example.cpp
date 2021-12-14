
#include <cstdint>
#include <cstdlib>

#include "../runtime/include/praas/global_memory.hpp"

#include <praas/output.hpp>

extern "C" int noop(
  uint8_t* input, size_t bytes, uint8_t*, size_t,
  const std::string& function_id, praas::output::Channel* out,
  praas::global::Memory* memory
)
{
  out->send(reinterpret_cast<char*>(input), bytes, function_id);
  return 0;
}


extern "C" int example(
  uint8_t*, size_t, uint8_t*, size_t,
  const std::string& function_id, praas::output::Channel* out,
  praas::global::Memory* memory
)
{
  out->send("Hello, World!", 14, function_id);
  return 0;
}

extern "C" int example2(
  uint8_t*, size_t, uint8_t*, size_t,
  const std::string& function_id, praas::output::Channel* out,
  praas::global::Memory* memory
)
{
  out->send("Hello, World!", 14, function_id);
  out->send("Hello, World2!", 15, function_id);
  return 0;
}

extern "C" int shm_example(
  uint8_t* buffer, size_t bytes, uint8_t* shm, size_t shm_bytes,
  const std::string& function_id, praas::output::Channel* out,
  praas::global::Memory* memory
)
{
  int b = std::min(bytes, shm_bytes);
  memcpy(shm, buffer, b);
  std::string msg = std::string{"Written "} + std::to_string(b) + " bytes";
  out->send(msg.c_str(), msg.length() + 1, function_id);
  return 0;
}

extern "C" int shm_read_example(
  uint8_t* buffer, size_t, uint8_t* shm, size_t shm_bytes,
  const std::string& function_id, praas::output::Channel* out,
  praas::global::Memory* memory
)
{
  size_t bytes_to_read = *reinterpret_cast<int32_t*>(buffer);
  int bytes_to_send = std::min(bytes_to_read, shm_bytes);
  out->send(reinterpret_cast<char*>(shm), bytes_to_send, function_id);
  return 0;
}

extern "C" int lm_benchmark_initialize(
  uint8_t* buffer, size_t bytes_to_read, uint8_t* shm, size_t shm_bytes,
  const std::string& function_id, praas::output::Channel* out,
  praas::global::Memory* memory
)
{
  //size_t bytes_to_read = *reinterpret_cast<int32_t*>(buffer);
  //uint8_t* data = new uint8_t[bytes_to_read]();
  //memcpy(data, buffer, bytes_to_read);
  bool result = memory->create("/obj1", praas::global::Object{buffer, bytes_to_read});
  //out->send(&bytes_to_read,sizeof(bytes_to_read),function_id);
  return 0;
}

extern "C" int lm_benchmark_read(
  uint8_t* buffer, size_t, uint8_t* shm, size_t shm_bytes,
  const std::string& function_id, praas::output::Channel* out,
  praas::global::Memory* memory
)
{
  int32_t reps = *reinterpret_cast<int32_t*>(buffer);
  uint64_t* results = reinterpret_cast<uint64_t*>(shm);
  memset(results, 0, sizeof(uint64_t)*(reps+1));

  for(int32_t i = 0; i < reps + 1; ++i) {
    auto _start = std::chrono::high_resolution_clock::now();
    auto obj = memory->get("/obj1");
    //std::cout << "get" << std::endl;
    bool ret = memory->put("/obj1", praas::global::Object{obj.value().data, obj.value().size});
    //std::cout << "put" << std::endl;
    auto _end = std::chrono::high_resolution_clock::now();
    uint64_t duration = std::chrono::duration_cast<std::chrono::nanoseconds>(_end - _start).count();
    results[i] = duration;
    if(!ret || !obj.has_value() || obj.value().size == 0)
      std::cerr << "Failure!" << std::endl;
    if(i == 0)
      std::cout << obj.has_value() << " " << obj.value().size << " " << results[i] << std::endl;
    //usleep(200);
  }

  out->send(reinterpret_cast<char*>(results), sizeof(uint64_t) * (reps+1), function_id);
  return 0;
}

