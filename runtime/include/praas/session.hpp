
#ifndef __RUNTIME_SESSION_HPP__
#define __RUNTIME_SESSION_HPP__

#include <cstdint>
#include <string>

namespace praas::session {

  struct SharedMemory {
    // memory block size
    int32_t size;
    // memory block
    void* _ptr;

    SharedMemory(int32_t size);
    ~SharedMemory();
  };

  struct Session {
    SharedMemory memory;
    int32_t max_functions;
    std::string session_id;
    bool ending;

    // FIXME: here we should have a fork of process with restricted permissions
    Session(std::string session_id, int32_t max_functions, int32_t memory_size);

    void run();
    void shutdown();
  };


};

#endif
