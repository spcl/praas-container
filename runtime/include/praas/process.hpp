
#ifndef __RUNTIME_PROCESS_HPP__
#define __RUNTIME_PROCESS_HPP__

#include "praas/messages.hpp"
#include <cstdint>
#include <vector>
#include <optional>

#include <sockpp/tcp_connector.h>

#include <praas/session.hpp>

namespace praas::process {

  enum class ErrorCode : int32_t {
    SUCCESS = 0,
    NO_MORE_CAPACITY = 1,
    SESSION_CONFLICT = 2,
    UNKNOWN_FAILURE
  };

  struct Process {
    bool _ending;
    int16_t _max_sessions;
    std::string _process_id;
    std::string _hole_puncher_address;
    std::vector<praas::session::SessionFork> _sessions;
    // Connect to process to allocate sessions.
    sockpp::tcp_connector _control_plane_socket;

    Process(
      std::string process_id, std::string hole_puncher_address,
      sockpp::tcp_connector && socket, int16_t max_sessions
    ):
      _ending(false),
      _max_sessions(max_sessions),
      _process_id(process_id),
      _hole_puncher_address(hole_puncher_address)
    {
      _control_plane_socket = std::move(socket); 
    }

    Process(Process && p) noexcept
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
      }
      return *this;
    }

    static std::optional<Process> create(
      std::string process_id, std::string ip_address, int32_t port,
      std::string hole_puncher_addr, int16_t max_sessions
    );
    void start();
    void shutdown();
    ErrorCode allocate_session(praas::messages::SessionRequestMsg&);
  };

};

#endif
