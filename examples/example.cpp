
#include <cstdint>
#include <cstdlib>

#include <praas/output.hpp>

extern "C" int example(uint8_t* buffer, size_t bytes, praas::output::Channel* out)
{
  out->send("Hello, World!", 14);
  return 0;
}

extern "C" int example2(uint8_t* buffer, size_t bytes, praas::output::Channel* out)
{
  out->send("Hello, World2!", 15);
  return 0;
}
