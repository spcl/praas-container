
#include <cstdint>
#include <cstdlib>

#include <praas/output.hpp>

extern "C" int example(
  uint8_t*, size_t, uint8_t*, size_t,
  const std::string& function_id, praas::output::Channel* out
)
{
  out->send("Hello, World!", 14, function_id);
  return 0;
}

extern "C" int example2(
  uint8_t*, size_t, uint8_t*, size_t,
  const std::string& function_id, praas::output::Channel* out
)
{
  out->send("Hello, World!", 14, function_id);
  out->send("Hello, World2!", 15, function_id);
  return 0;
}

extern "C" int shm_example(
  uint8_t* buffer, size_t bytes, uint8_t* shm, size_t shm_bytes,
  const std::string& function_id, praas::output::Channel* out
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
  const std::string& function_id, praas::output::Channel* out
)
{
  size_t bytes_to_read = *reinterpret_cast<int32_t*>(buffer);
  int bytes_to_send = std::min(bytes_to_read, shm_bytes);
  out->send(reinterpret_cast<char*>(shm), bytes_to_send, function_id);
  return 0;
}
