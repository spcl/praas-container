
#include "praas/session.hpp"
#include <optional>
#include <sockpp/inet_address.h>
#include <sockpp/tcp_connector.h>
#include <sockpp/tcp_socket.h>
#include <spdlog/spdlog.h>

#include <praas/process.hpp>
#include <praas/session.hpp>

namespace praas::process {
 
  std::optional<Process> Process::create(std::string ip_address, int32_t port, int16_t max_sessions)
  {
    try {
      sockpp::tcp_connector socket(sockpp::inet_address(ip_address, port));
      if(socket.connect(sockpp::inet_address(ip_address, port)))
        return std::make_optional<Process>(std::move(socket), max_sessions);
      else
        return std::optional<Process>();
    } catch (...) {
      return std::optional<Process>();
    }
  }

  void Process::start()
  {
    if (!_control_plane_socket)
      spdlog::error("Couldn't connect to control plane! {}", _control_plane_socket.last_error_str());

    // FIXME: here also accept new requests to connect from data plane (user).
    spdlog::info("Process begins work!");
    praas::session::Request req;
    while(!_ending) {

      // Read requests to allocate new sesions
      ssize_t read_size = _control_plane_socket.read(req.buf, req.REQUEST_BUF_SIZE);

      if(_ending)
        break;
      if (read_size == -1) {
        spdlog::error(
          "Error accepting incoming session allocation request, reason: {}",
          _control_plane_socket.last_error_str()
        );
        continue;
      }

      spdlog::debug(
        "Received session allocation request: max functions {}, memory {}, session id {}",
        req.max_functions(), req.memory_size(), req.session_id()
      );
      ErrorCode result = ErrorCode::SUCCESS;
      // Check if we have capacity
      if(_sessions.size() >= _max_sessions) {
        result = ErrorCode::NO_MORE_CAPACITY;
      } else {
        // Verify conflicts - simple search
        auto it = std::find_if(
          _sessions.begin(), _sessions.end(),
          [&req](const praas::session::Session & s) {
            return s.session_id == req.session_id();
          }
        );
        if(it != _sessions.end()) {
          result = ErrorCode::SESSION_CONFLICT; 
        }
        // Now we can start a session
        else {
          _sessions.emplace_back(req.session_id(), req.max_functions(), req.memory_size());
        }
      }
      // Notify the control plane about success/failure
      _control_plane_socket.write(&result, sizeof(ErrorCode));
    }
    spdlog::info("Process ends work!");
  }

  void Process::shutdown()
  {
    spdlog::info("Shutdown process!");
    _ending = true;
    _control_plane_socket.shutdown(); 
    for(praas::session::Session & session: _sessions)
      session.shutdown();
  }

}

