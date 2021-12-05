
#ifndef __RUNTIME_SESSION_HPP__
#define __RUNTIME_SESSION_HPP__

#include <cstdint>
#include <string>

namespace praas::session {

  struct Request {

    // 4 bytes memory size (kB)
    // 4 bytes max_functions
    // 16 bytes session ID
    static constexpr int REQUEST_BUF_SIZE = 24;
    int8_t buf[REQUEST_BUF_SIZE];

    int32_t memory_size()
    {
      return *reinterpret_cast<int32_t*>(buf);
    }

    int32_t max_functions()
    {
      return *reinterpret_cast<int32_t*>(buf + 4);
    }

    std::string session_id()
    {
      return std::string{reinterpret_cast<char*>(buf + 8), 16};
    }

  };

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
