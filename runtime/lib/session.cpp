
#include <charconv>
#include <future>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <spdlog/spdlog.h>
#include <sockpp/tcp_connector.h>

#include <praas/session.hpp>
#include "praas/messages.hpp"

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
    session_id(session_id),
    ending(false)
  {
    // FIXME: open shared memory
    // FIXME: max functions
  }

  void Session::process_invocation(
    ssize_t bytes, praas::messages::FunctionRequestMsg & msg, sockpp::tcp_connector & connection
  )
  {
    std::string fname = msg.function_id();
    spdlog::info("Invoking function {} with {} payload", fname, msg.payload());
    // FIXME: function invocation
    std::unique_ptr<int8_t[]> arr{new int8_t[msg.payload()]};
    ssize_t payload_bytes = connection.read(arr.get(), msg.payload());
    spdlog::info("Function {}, received {} payload", fname, payload_bytes);
    spdlog::info("Invoked function {} with {} payload", fname, msg.payload());
  }

  void Session::start(std::string control_plane_addr)
  {
    auto [ip_address, port_str] = split(control_plane_addr, ':');
    int port;
    // https://stackoverflow.com/questions/56634507/safely-convert-stdstring-view-to-int-like-stoi-or-atoi
    std::from_chars(port_str.data(), port_str.data() + port_str.size(), port);
    sockpp::tcp_connector control_plane_connection;
    try {
      if(!control_plane_connection.connect(sockpp::inet_address(std::string{ip_address}, port))) {
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
      control_plane_connection.write(msg.data, bytes_size);
    }

    // FIXME: Connect to control plane and data plane for invocations
    spdlog::info("Session {} begins work!", this->session_id);

    // FIXME: Read from data plane
    // FIXME: ring buffer
    praas::messages::RecvMessageBuffer msg;
    while(!ending) {
      ssize_t bytes = control_plane_connection.read(msg.data, msg.REQUEST_BUF_SIZE);
      if(ending)
        break;
      // EOF
      if(bytes == 0) {
        spdlog::debug(
          "End of file on connection with {}.",
          control_plane_connection.peer_address().to_string()
        );
        return;
      }
      // Incorrect read
      else if(bytes == -1) {
        spdlog::debug(
          "Incorrect read on connection with {}.",
          control_plane_connection.peer_address().to_string()
        );
        return;
      }

      // Parase the message
      std::unique_ptr<praas::messages::RecvMessage> ptr = msg.parse(bytes);

      if(!ptr || ptr->type() != praas::messages::RecvMessage::Type::INVOCATION_REQUEST) {
        spdlog::error("Unknown request");
        continue;
      }

      process_invocation(
        bytes,
        dynamic_cast<praas::messages::FunctionRequestMsg&>(*ptr.get()),
        control_plane_connection
      );
    }
    spdlog::info("Session {} ends work!", this->session_id);
  }

  void Session::shutdown()
  {
    // FIXME: catch signal, graceful quit
    ending = true;
  }
};
