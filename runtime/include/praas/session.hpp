
#ifndef __RUNTIME_SESSION_HPP__
#define __RUNTIME_SESSION_HPP__

#include <cstdint>
#include <string>

#include <sockpp/tcp_connector.h>

#include <praas/sockets.hpp>
#include <praas/buffer.hpp>

namespace praas::messages {
  struct FunctionRequestMsg;
}

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
    //SharedMemory memory;
    praas::buffer::BufferQueue<int8_t> _buffers;
    int32_t max_functions;
    std::string session_id;
    bool ending;

    // FIXME: shared memory
    Session(std::string session_id, int32_t max_functions, int32_t memory_size);
    void start(std::string control_plane_addr);
    void shutdown();
    void process_invocation(
      std::string fname, ssize_t bytes, praas::buffer::Buffer<int8_t> buf, sockpp::tcp_connector & connection
    );
  };

  struct SessionFork {
    SharedMemory memory;
    std::string session_id;
    pid_t child_pid;

    // FIXME: here we should have a fork of process with restricted permissions
    SessionFork(std::string session_id, int32_t max_functions, int32_t memory_size);
    void fork(std::string controller_address);
    void shutdown();
  };


};

#endif
