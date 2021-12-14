
#include <charconv>
#include <future>
#include <utility>

#include <sys/mman.h>
#include <fcntl.h>

#include <sockpp/inet_address.h>
#include <sockpp/tcp_socket.h>
#include <sockpp/tcp_connector.h>
#include <tcpunch.h>

#include <praas/session.hpp>
#include <praas/buffer.hpp>
#include <praas/function.hpp>
#include <praas/swapper.hpp>

#include "../include/praas/global_memory.hpp"

using praas::buffer::BufferQueue;

namespace praas::session {

  std::tuple<std::string_view, std::string_view> split(std::string & str, char split_character)
  {
    auto pos = str.find(split_character);
    if(pos == std::string::npos)
      return std::make_tuple("", "");
    else
      return std::make_tuple(
        std::string_view(str.data(), pos),
        std::string_view(str.data() + pos + 1, str.length() - pos - 1)
      );
  }

  SharedMemory::SharedMemory(
    const std::string& path, int file_descriptor,
    int32_t size, void* ptr, bool owned
  ):
    _size(size),
    _fd(file_descriptor),
    _path(path),
    _ptr(ptr),
    _owned(owned)
  {}

  SharedMemory::SharedMemory(SharedMemory && obj)
  {
    this->_size = std::move(obj._size);
    this->_fd = std::move(obj._fd);
    this->_path = std::move(obj._path);
    this->_ptr = std::move(obj._ptr);
    this->_owned = std::move(obj._owned);

    obj._size = -1;
    obj._fd = -1;
    obj._path = "";
    obj._ptr = nullptr;
    obj._owned = false;
  }

  SharedMemory& SharedMemory::operator=(SharedMemory && obj)
  {
    if(this != &obj) {
      this->_size = std::move(obj._size);
      this->_fd = std::move(obj._fd);
      this->_path = std::move(obj._path);
      this->_ptr = std::move(obj._ptr);
      this->_owned = std::move(obj._owned);

      obj._size = -1;
      obj._fd = -1;
      obj._path = "";
      obj._ptr = nullptr;
      obj._owned = false;
    }

    return *this;
  }

  SharedMemory::~SharedMemory()
  {
    if(_ptr) {
      if(munmap(_ptr, _size) == -1) {
        spdlog::error("Failed to unmap shared memory!");
      } else {
        spdlog::debug("Unmap shared memory");
      }
    }
    if(_owned && _fd != -1) {
      if(shm_unlink(_path.c_str()) == -1) {
        spdlog::error("Failed to unlink shared memory at {}! Error: {}", _path, strerror(errno));
      } else {
        spdlog::debug("Unlink shared memory at {}.", _path);
      }
    }
  }

  std::optional<SharedMemory> SharedMemory::create(
    std::string session_id, int32_t size,
    swapper::S3Swapper & s3_swapper
  )
  {
    session_id.insert(0, "/");
    int fd = shm_open(session_id.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if(fd == -1) {
      spdlog::error(
        "Couldn't allocate shared memory at {}! Reason: {}",
        session_id,
        strerror(errno)
      );
      return std::optional<SharedMemory>{};
    }

    // Swapped memory has negative size
    bool swap_in = size < 0;
    size *= swap_in ? -1 : 1;

    // kbytes -> bytes
    size *= 1024;
    if(ftruncate(fd, size) == -1) {
      spdlog::error("Couldn't resize shared memory!");
      return std::optional<SharedMemory>{};
    }

    void* ptr = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);
    if(ptr == MAP_FAILED) {
      spdlog::error("Failed to map shared memory");
      return std::optional<SharedMemory>{};
    }

    if(swap_in)
      if(!s3_swapper.swap_in(session_id, static_cast<char*>(ptr), size)) {
        spdlog::error("Swapping in failed, unlink!");
        munmap(ptr, size);
        shm_unlink(session_id.c_str());
        return std::optional<SharedMemory>{};
      }

    spdlog::debug("Created {} kbytes of shared memory at {} for session_id {}", size, fmt::ptr(ptr), session_id);
    return std::optional<SharedMemory>{std::in_place_t{}, session_id, fd, size, ptr, true};
  }

  std::optional<SharedMemory> SharedMemory::open(std::string session_id, int32_t size)
  {
    session_id.insert(0, "/");
    int fd = shm_open(session_id.c_str(), O_RDWR, S_IRUSR | S_IWUSR);
    if(fd == -1) {
      spdlog::error(
        "Couldn't open shared memory at {}! Reason: {}",
        session_id,
        strerror(errno)
      );
      return std::optional<SharedMemory>{};
    }

    void* ptr = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);
    if(ptr == MAP_FAILED) {
      spdlog::error("Failed to map shared memory");
      return std::optional<SharedMemory>{};
    }

    spdlog::debug("Opened {} bytes of shared memory at {} for session {}", size, fmt::ptr(ptr), session_id);
    return std::optional<SharedMemory>{std::in_place_t{}, session_id, fd, size, ptr};
  }

  SessionFork::SessionFork(std::string session_id, int32_t max_functions, int32_t memory_size):
    session_id(session_id),
    max_functions(max_functions),
    memory_size(memory_size),
    child_pid(-1)
  {}

  bool SessionFork::fork(std::string controller_address, std::string hole_puncher_address,
      swapper::S3Swapper& s3_swapper)
  {
    std::optional<SharedMemory> shm = SharedMemory::create(session_id, memory_size, s3_swapper);
    if(!shm.has_value()) {
      spdlog::error("Couldn't fork process - no shared memory!");
      return false;
    }

    // FIXME: here we should have a fork of process with restricted permissions
    child_pid = vfork();
    if(child_pid == 0) {
      auto out_file = ("session_" + session_id);
      int fd = open(out_file.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
      dup2(fd, 1);
      dup2(fd, 2);
      // Take memory from shm object -> it's in bytes now and definetely positive
      std::string memory_size_str = std::to_string(shm.value().size());
      std::string max_functions_str = std::to_string(max_functions);
      const char * argv[] = {
        "/dev-praas/bin/runtime_session",
        "--control-plane-addr", controller_address.c_str(),
        "--hole-puncher-addr", hole_puncher_address.c_str(),
        "--session-id", session_id.c_str(),
        "--shared-memory-size", memory_size_str.c_str(),
        "--max-functions", max_functions_str.c_str(),
        "-v", nullptr
      };
      int ret = execv(argv[0], const_cast<char**>(&argv[0]));
      if(ret == -1) {
        spdlog::error("Session Executor process failed {}, reason {}", errno, strerror(errno));
        exit(1);
      }
    } else {
      spdlog::debug("Launched session executor for session {}, PID {}", session_id, child_pid);
      this->memory = std::move(shm.value());
      return true;
    }
    // Avoid compiler errors
    return true;
  }

  void SessionFork::shutdown()
  {
    // FIXME: kill the process
  }

  Session::Session(std::string session_id, int32_t max_functions, int32_t memory_size):
    // FIXME: buffer size - configurable?
    _buffers(2*max_functions, 6*1024*1024),
    _pool(max_functions),
    _memory_size(memory_size),
    session_id(session_id),
    ending(false)
  {
    // FIXME: open shared memory
    // FIXME: max functions
    praas::function::FunctionWorkers::init(_library, _buffers);
  }

  Session::~Session()
  {
    praas::function::FunctionWorkers::free();
  }

  void Session::start(std::string control_plane_addr, std::string hole_puncher_addr)
  {
    // Allocate
    std::optional<SharedMemory> shm = SharedMemory::open(session_id, _memory_size);
    if(!shm.has_value()) {
      spdlog::error("Couldn't open shared memory!");
      return;
    }
    this->_shm = std::move(shm.value());


    auto [ip_address, port_str] = split(control_plane_addr, ':');
    int port;
    // https://stackoverflow.com/questions/56634507/safely-convert-stdstring-view-to-int-like-stoi-or-atoi
    std::from_chars(port_str.data(), port_str.data() + port_str.size(), port);
    sockpp::tcp_connector connection;
    try {
      spdlog::debug("Connecting to control plane at {}:{}.", ip_address, port);
      if(!connection.connect(sockpp::inet_address(std::string{ip_address}, port))) {
        spdlog::error("Couldn't connect to control plane at {}.", control_plane_addr);
        return;
      }
    } catch(...) {
      spdlog::error("Couldn't connect to control plane at {}.", control_plane_addr);
      return;
    }

    // Announce our presence to the control plane and get the invocation.
    {
      praas::messages::SendMessage msg;
      msg.fill_session_identification(this->session_id);
      connection.write(msg.data, msg.BUF_SIZE);
    }

    sockpp::tcp_connector mem_conn;
    mem_conn.connect(sockpp::inet_address("127.0.0.1", 7000));
    praas::global::Memory memory{&mem_conn};

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

    receive(connection, output_channel, memory, msg);
    while(!ending) {
      if(!receive(_data_plane_socket, output_channel, memory, msg))
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
