
#ifndef __RUNTIME_PROCESS_HPP__
#define __RUNTIME_PROCESS_HPP__

#include <cstdint>
#include <vector>
#include <optional>

#include <sockpp/tcp_connector.h>

#include <praas/session.hpp>
#include <praas/swapper.hpp>

namespace praas::process {

  enum class ErrorCode : int32_t {
    SUCCESS = 0,
    NO_MORE_CAPACITY = 1,
    SESSION_CONFLICT = 2,
    UNKNOWN_FAILURE
  };

  // This works because we have only a single instance of this class in a single process.
  void session_shutdown_handler(int signo, siginfo_t *si, void *ucontext);

  struct Process {
    bool _ending;
    int16_t _max_sessions;
    std::string _process_id;
    std::string _hole_puncher_address;
    std::vector<praas::session::SessionFork> _sessions;
    // Connect to process to allocate sessions.
    sockpp::tcp_connector _control_plane_socket;
    // Session swapper
    praas::swapper::S3Swapper _swapper;
    // Mutex used to manage active sessions
    std::mutex _sessions_mutex;

    Process(
      std::string process_id, std::string hole_puncher_address,
      sockpp::tcp_connector && socket, int16_t max_sessions,
      bool verbose
    ):
      _ending(false),
      _max_sessions(max_sessions),
      _process_id(process_id),
      _hole_puncher_address(hole_puncher_address),
      _swapper(verbose)
    {
      _control_plane_socket = std::move(socket); 

      // FIXME: Signal handling - make singleton
      _instance = this;

      struct sigaction sa;
      memset(&sa, 0, sizeof(struct sigaction));
      sigemptyset(&sa.sa_mask);
      sa.sa_sigaction = session_shutdown_handler;
      // Tell the kernel to restart interrupted system calls on SIGCHLD.
      // We don't want to complicate socket handling because of a session shutdown.
      sa.sa_flags   = SA_SIGINFO | SA_RESTART;
      sigaction(SIGCHLD, &sa, NULL);
    }

    ~Process()
    {
      spdlog::error("Destruct {}", fmt::ptr(this));
    }

    Process(Process && p) noexcept:
      _swapper(std::move(p._swapper))
    {
      this->_control_plane_socket = std::move(p._control_plane_socket);
      this->_ending = std::move(p._ending);
      this->_max_sessions = std::move(p._max_sessions);
      this->_sessions = std::move(p._sessions);
      this->_process_id = std::move(p._process_id);
      this->_hole_puncher_address = std::move(p._hole_puncher_address);
    }

    Process & operator=(Process && p) noexcept
    {
      if(this != &p) {
        this->_control_plane_socket.shutdown();
        this->_sessions.clear();

        this->_control_plane_socket = std::move(p._control_plane_socket);
        this->_ending = std::move(p._ending);
        this->_max_sessions = std::move(p._max_sessions);
        this->_sessions = std::move(p._sessions);
        this->_process_id = std::move(p._process_id);
        this->_hole_puncher_address = std::move(p._hole_puncher_address);
        this->_swapper = std::move(p._swapper);
      }
      return *this;
    }

    bool enable_swapping()
    {
      return _swapper.enable_swapping();
    }

    static void create(
      std::optional<Process> &,
      std::string process_id, std::string ip_address, int32_t port,
      std::string hole_puncher_addr, int16_t max_sessions,
      bool verbose = false
    );
    void start();
    void shutdown();
    ErrorCode allocate_session(praas::messages::SessionRequestMsg&);
    bool swapping_enabled();
    void session_deleted(int pid);

    static Process* _instance;
    static Process* get_instance();
  };

}

#endif
