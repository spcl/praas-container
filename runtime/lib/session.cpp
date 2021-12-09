
#include <charconv>
#include <future>

#include <sys/mman.h>
#include <fcntl.h>

#include <sockpp/inet_address.h>
#include <sockpp/tcp_socket.h>
#include <sockpp/tcp_connector.h>
#include <tcpunch.h>

#include <praas/session.hpp>
#include <praas/buffer.hpp>
#include <praas/function.hpp>

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

  void SessionFork::fork(std::string controller_address, std::string hole_puncher_address)
  {
    child_pid = vfork();
    if(child_pid == 0) {
      auto out_file = ("session_" + session_id);
      int fd = open(out_file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
      dup2(fd, 1);
      dup2(fd, 2);
      const char * argv[] = {
        "/dev-praas/bin/runtime_session",
        "--control-plane-addr", controller_address.c_str(),
        "--hole-puncher-addr", hole_puncher_address.c_str(),
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
    _pool(max_functions),
    session_id(session_id),
    ending(false)
  {
    // FIXME: open shared memory
    // FIXME: max functions
    praas::function::FunctionWorkers::init(_library);
  }

  Session::~Session()
  {
    praas::function::FunctionWorkers::free();
  }

  void Session::start(std::string control_plane_addr, std::string hole_puncher_addr)
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

    // Translate the address.
    auto addr = sockpp::inet_address::resolve_name(hole_puncher_addr);
    char addrstr[16];
    inet_ntop(AF_INET, &addr, addrstr, 16);
    spdlog::info("Attempt pairing on session {}, connect to resolved name {}", session_id, addrstr);
    int sock_fd = pair(this->session_id.c_str(), addrstr);
    _data_plane_socket = sockpp::tcp_socket{sock_fd};
    spdlog::info(
      "Session {} connected data plane to: {}",
      this->session_id, _data_plane_socket.peer_address().to_string()
    );

    // Begin receiving from the control plane, and then switch to the data plane.
    praas::messages::RecvMessageBuffer msg;
    praas::output::Channel output_channel{_data_plane_socket};

    receive(connection, output_channel, msg);
    while(!ending) {
      if(!receive(_data_plane_socket, output_channel, msg))
        break;
    }

    spdlog::info("Session {} ends work!", this->session_id);
    // Wait for threads to finish before destroying memory
    _pool.wait_for_tasks();
    spdlog::info("Tasks finished on session {}", session_id);
  }

  void Session::shutdown()
  {
    // FIXME: catch signal, graceful quit
    ending = true;
    spdlog::info("Session is closed down!");
  }

}
