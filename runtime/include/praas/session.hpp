
#ifndef __RUNTIME_SESSION_HPP__
#define __RUNTIME_SESSION_HPP__

#include <cstdint>
#include <string>
#include <bitset>

#include <sockpp/tcp_connector.h>
#include <thread_pool.hpp>
#include <spdlog/spdlog.h>

#include <praas/buffer.hpp>
#include <praas/messages.hpp>
#include <praas/function.hpp>
#include <praas/output.hpp>

namespace praas::messages {
  struct FunctionRequestMsg;
}

namespace {

  template <typename Dest, typename Source,  typename Deleter>
  std::unique_ptr<Dest, Deleter> dynamic_cast_unique(std::unique_ptr<Source, Deleter> && ptr)
  {
    if (Dest* casted_ptr = dynamic_cast<Dest*>(ptr.get()))
    {
      std::unique_ptr<Dest, Deleter> result(casted_ptr, std::move(ptr.get_deleter()));
      ptr.release();
      return result;
    }
    return nullptr;
  }

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
    praas::buffer::BufferQueue<uint8_t> _buffers;
    thread_pool _pool;
    int32_t max_functions;
    std::string session_id;
    bool ending;
    praas::function::FunctionsLibrary _library;
    sockpp::tcp_socket _data_plane_socket;

    // FIXME: shared memory
    Session(std::string session_id, int32_t max_functions, int32_t memory_size);
    ~Session();
    void start(std::string control_plane_addr, std::string hole_puncher_addr);
    void shutdown();

    template<typename InSocket>
    bool receive(
      InSocket && connection, praas::output::Channel & out_connection, praas::messages::RecvMessageBuffer & msg
    )
    {
      spdlog::debug(
        "Session {} polls for new invocations from {}",
        session_id,
        connection.peer_address().to_string()
      );
      ssize_t bytes = connection.read(msg.data, 22);
      spdlog::debug(
        "Session {} polled {} bytes of new invocations from {}",
        bytes,
        session_id,
        connection.peer_address().to_string()
      );

      // EOF or incorrect read
      if(bytes == 0) {
        spdlog::debug(
          "End of file on connection with {}.",
          connection.peer_address().to_string()
        );
        // FIXME: handle closure
        return false;
      } else if(bytes == -1) {
        spdlog::debug(
          "Incorrect read on connection with {}.",
          connection.peer_address().to_string()
        );
        return true;
      }

      // Parse the message
      std::unique_ptr<praas::messages::RecvMessage> ptr = msg.parse(bytes);
      if(!ptr || ptr->type() != praas::messages::RecvMessage::Type::INVOCATION_REQUEST) {
        spdlog::error("Unknown request");
        out_connection.send_error(praas::messages::FunctionMessage::Status::UNKNOWN_REQUEST);
        return true;
      }
      auto parsed_msg = dynamic_cast_unique<praas::messages::FunctionRequestMsg>(std::move(ptr));

      if(parsed_msg->payload() == 0) {
        spdlog::info("No invocation data");
        return true;
      }

      // Receive payload
      auto buf = _buffers.retrieve_buffer(parsed_msg->payload());
      if(buf.size == 0) {
        spdlog::error("Not enough memory capacity to handle the invocation!");
        out_connection.send_error(praas::messages::FunctionMessage::Status::OUT_OF_MEMORY);
        return true;
      }
      ssize_t payload_bytes = connection.read(buf.val, parsed_msg->payload());
      spdlog::debug("Function {}, received {} payload", parsed_msg->function_id(), payload_bytes);

      // Invoke function
      _pool.push_task(
        praas::function::FunctionWorker::invoke,
        parsed_msg->function_id(),
        payload_bytes,
        buf,
        &out_connection
      );
      return true;
    }
  };

  struct SessionFork {
    SharedMemory memory;
    std::string session_id;
    pid_t child_pid;

    // FIXME: here we should have a fork of process with restricted permissions
    SessionFork(std::string session_id, int32_t max_functions, int32_t memory_size);
    void fork(std::string controller_address, std::string hole_puncher_address);
    void shutdown();
  };


};

#endif
