
#include "praas/buffer.hpp"
#include <charconv>
#include <future>
#include <sys/mman.h>

#include <spdlog/spdlog.h>
#include <sockpp/tcp_connector.h>

#include <praas/session.hpp>
#include <praas/messages.hpp>

using praas::buffer::BufferQueue;

namespace praas::session {

  std::tuple<std::string_view, std::string_view> split(std::string & str, char split_character)
  {
    ssize_t pos = str.find(split_character);
    if(pos == std::string::npos)
      return std::make_tuple("", "");
    else
      return std::make_tuple(
        std::string_view(str.data(), pos),
        std::string_view(str.data() + pos + 1, str.length() - pos - 1)
      );
  }

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

  SessionFork::SessionFork(std::string session_id, int32_t max_functions, int32_t memory_size):
    memory(memory_size),
    session_id(session_id),
    child_pid(-1)
  {}

  void SessionFork::fork(std::string controller_address)
  {
    child_pid = vfork();
    if(child_pid == 0) {
      const char * argv[] = {
        "/dev-praas/bin/runtime_session",
        "-a", controller_address.c_str(),
        "--session-id", session_id.c_str(),
        "--shared-memory", "FIXME",
        "-v", nullptr
      };
      int ret = execv(argv[0], const_cast<char**>(&argv[0]));
      if(ret == -1) {
        spdlog::error("Executor process failed {}, reason {}", errno, strerror(errno));
        exit(1);
      }
    } else {
      spdlog::debug("Launched session executor for session {}, PID {}", session_id, child_pid);
    }
  }

  void SessionFork::shutdown()
  {
    // FIXME: kill the process
  }

  Session::Session(std::string session_id, int32_t max_functions, int32_t memory_size):
    // FIXME: parameters - should depend on max functions
    _buffers(10, 1024*1024),
    session_id(session_id),
    ending(false)
  {
    // FIXME: open shared memory
    // FIXME: max functions
  }

  void Session::process_invocation(
    std::string fname, ssize_t bytes, praas::buffer::Buffer<int8_t> buf, sockpp::tcp_connector & connection
  )
  {
    spdlog::info("Invoking function {} with {} payload", fname, bytes);
    spdlog::info("Invoked function {} with {} payload", fname, bytes);
  }

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

  void Session::start(std::string control_plane_addr)
  {
    auto [ip_address, port_str] = split(control_plane_addr, ':');
    int port;
    // https://stackoverflow.com/questions/56634507/safely-convert-stdstring-view-to-int-like-stoi-or-atoi
    std::from_chars(port_str.data(), port_str.data() + port_str.size(), port);
    sockpp::tcp_connector connection;
    try {
      if(!connection.connect(sockpp::inet_address(std::string{ip_address}, port))) {
        spdlog::error("Couldn't connect to control plane at {}.", control_plane_addr);
        return;
      }
    } catch(...) {
      spdlog::error("Couldn't connect to control plane at {}.", control_plane_addr);
      return;
    }
    spdlog::debug("Connected to control plane at {}:{}.", ip_address, port);

    // Announce our presence to the control plane and get the invocation.
    {
      praas::messages::SendMessage msg;
      ssize_t bytes_size = msg.fill_session_identification(this->session_id);
      connection.write(msg.data, bytes_size);
    }

    spdlog::info("Session {} begins work!", this->session_id);

    // FIXME: Read from data plane
    praas::messages::RecvMessageBuffer msg;
    while(!ending) {

      ssize_t bytes = connection.read(msg.data, msg.REQUEST_BUF_SIZE);

      // EOF or incorrect read
      if(bytes == 0) {
        spdlog::debug(
          "End of file on connection with {}.",
          connection.peer_address().to_string()
        );
        // FIXME: handle closure
        break;
      } else if(bytes == -1) {
        spdlog::debug(
          "Incorrect read on connection with {}.",
          connection.peer_address().to_string()
        );
        continue;
      }

      // Parse the message
      std::unique_ptr<praas::messages::RecvMessage> ptr = msg.parse(bytes);
      if(!ptr || ptr->type() != praas::messages::RecvMessage::Type::INVOCATION_REQUEST) {
        spdlog::error("Unknown request");
        continue;
      }
      auto msg = dynamic_cast_unique<praas::messages::FunctionRequestMsg>(std::move(ptr));

      // Receive payload
      auto buf = _buffers.retrieve_buffer(msg->payload());
      if(buf.size == 0) {
        spdlog::error("Not enough memory capacity to handle the invocation!");
        // FIXME: notify client
        continue;
      }
      ssize_t payload_bytes = connection.read(buf.val, msg->payload());
      spdlog::info("Function {}, received {} payload", msg->function_id(), payload_bytes);

      // Invoke function
      process_invocation(
        msg->function_id(),
        payload_bytes,
        buf,
        connection
      );
    }

    spdlog::info("Session {} ends work!", this->session_id);
  }

  void Session::shutdown()
  {
    // FIXME: catch signal, graceful quit
    ending = true;
  }

}
