
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

    // memory block

  };

  struct Session {
    SharedMemory memory;

  };


};

#endif
