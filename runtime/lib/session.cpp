
#include <sys/mman.h>

#include <praas/session.hpp>

namespace praas::session {

  SharedMemory::SharedMemory(int32_t size):
    size(size)
  {
    // Shared between processes?
    _ptr = mmap(nullptr, size * 1024, PROT_WRITE, MAP_SHARED, 0, 0);
  }

  SharedMemory::~SharedMemory()
  {
    munmap(_ptr, size);
  }

  Session::Session(std::string session_id, int32_t max_functions, int32_t memory_size):
    memory(memory_size),
    max_functions(max_functions),
    session_id(session_id),
    ending(false)
  {
    run();
  }

  void Session::run()
  {
    // FIXME: Connect to control plane and data plane for invocations
    //while(!ending) {

    //}
  }

  void Session::shutdown()
  {
    ending = true;
  }
};
