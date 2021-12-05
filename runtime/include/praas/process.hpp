
#ifndef __RUNTIME_PROCESS_HPP__
#define __RUNTIME_PROCESS_HPP__

#include <cstdint>
#include <vector>
#include <optional>

#include <sockpp/tcp_connector.h>

#include <praas/session.hpp>

namespace praas::process {

  enum class ErrorCode : int32_t {
    SUCCESS = 0,
    NO_MORE_CAPACITY = 1,
    SESSION_CONFLICT
  };

  struct Process {
    bool _ending;
    int16_t _max_sessions;
    std::vector<praas::session::Session> _sessions;
    // Connect to process to allocate sessions.
    sockpp::tcp_connector _control_plane_socket;

    Process(sockpp::tcp_connector && socket, int16_t max_sessions):
      _ending(false),
      _max_sessions(max_sessions)
    {
      _control_plane_socket = std::move(socket); 
    }

    Process(Process && p) noexcept
    {
      this->_control_plane_socket = std::move(p._control_plane_socket);
      this->_ending = std::move(p._ending);
      this->_max_sessions = std::move(p._max_sessions);
      this->_sessions = std::move(p._sessions);
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
      }
      return *this;
    }


    static std::optional<Process> create(std::string ip_address, int32_t port, int16_t max_sessions);
    void start();
    void shutdown();
  };

};

#endif
