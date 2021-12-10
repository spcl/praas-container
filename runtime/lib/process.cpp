
#include "praas/session.hpp"
#include <optional>
#include <sockpp/inet_address.h>
#include <sockpp/tcp_connector.h>
#include <sockpp/tcp_socket.h>
#include <spdlog/spdlog.h>

#include <praas/process.hpp>
#include <praas/session.hpp>
#include <praas/messages.hpp>

namespace praas::process {

  Process* Process::_instance = nullptr;
 
  Process* Process::get_instance()
  {
    return _instance;
  }

  void session_shutdown_handler(int signo, siginfo_t *si, void *ucontext)
  {
    // FIXME: handle session swapping
    spdlog::debug("A session shutdown on PID {}", si->si_pid);
  }

  std::optional<Process> Process::create(
    std::string process_id, std::string ip_address, int32_t port,
    std::string hole_puncher_addr, int16_t max_sessions
  )
  {
    try {
      sockpp::tcp_connector socket;
      if(socket.connect(sockpp::inet_address(ip_address, port))) {
        spdlog::debug(
          "Succesful connection to control plane {}:{} from {} on process {}",
          ip_address, port, socket.address().to_string(), process_id
        );
        return std::make_optional<Process>(process_id, hole_puncher_addr, std::move(socket), max_sessions);
      } else
        return std::optional<Process>();
    } catch (...) {
      return std::optional<Process>();
    }
  }

  void Process::start()
  {
    if (!_control_plane_socket)
      spdlog::error("Couldn't connect to control plane! {}", _control_plane_socket.last_error_str());

    // Now let's identify ourselves to the control plane
    {
      praas::messages::SendMessage msg;
      ssize_t bytes_size = msg.fill_process_identification(this->_process_id);
      _control_plane_socket.write(msg.data, bytes_size);
    }

    // FIXME: here also accept new requests to connect from data plane (user).
    spdlog::info("Process {} begins work!", this->_process_id);
    praas::messages::RecvMessageBuffer msg;
    while(!_ending) {

      // Read requests to allocate new sessions
      ssize_t read_size = _control_plane_socket.read(msg.data, msg.EXPECTED_LENGTH);

      if(_ending)
        break;
      if (read_size == -1) {
        spdlog::error(
          "Error accepting incoming session allocation request, reason: {}",
          _control_plane_socket.last_error_str()
        );
        continue;
      } else if (read_size == 0){
        spdlog::debug(
          "End of file on connection with {}.",
          _control_plane_socket.peer_address().to_string()
        );
        break;
      }

      std::unique_ptr<praas::messages::RecvMessage> req = msg.parse(read_size);
      if(!req) {
        spdlog::error("Incorrect message of size {} received.", read_size);
        continue;
      }
      ErrorCode result = ErrorCode::UNKNOWN_FAILURE;
      if(req.get()->type() == praas::messages::RecvMessage::Type::SESSION_REQUEST) {
        result = allocate_session(*dynamic_cast<praas::messages::SessionRequestMsg*>(req.get()));
      } else {
        spdlog::error("Unknown type of request!");
      }
      // Notify the control plane about success/failure
      _control_plane_socket.write(&result, sizeof(ErrorCode));
    }
    spdlog::info("Process ends work!");
  }

  ErrorCode Process::allocate_session(praas::messages::SessionRequestMsg& msg)
  {
    spdlog::debug(
      "Received session allocation request: max functions {}, memory {}, session id {}",
      msg.max_functions(), msg.memory_size(), msg.session_id()
    );

    ErrorCode result = ErrorCode::SUCCESS;
    // Check if we have capacity
    if(_sessions.size() >= _max_sessions) {
      result = ErrorCode::NO_MORE_CAPACITY;
    } else {
      // Verify conflicts - simple search
      auto it = std::find_if(
        _sessions.begin(), _sessions.end(),
        [&msg](const praas::session::SessionFork & s) {
          return s.session_id == msg.session_id();
        }
      );
      if(it != _sessions.end()) {
        result = ErrorCode::SESSION_CONFLICT; 
      }
      // Now we can start a session
      else {
        _sessions.emplace_back(msg.session_id(), msg.max_functions(), msg.memory_size());
        _sessions.back().fork(_control_plane_socket.peer_address().to_string(), _hole_puncher_address);
      }
    }
    return result;
  }

  void Process::shutdown()
  {
    spdlog::info("Shutdown process!");
    _ending = true;
    for(praas::session::SessionFork & session: _sessions)
      session.shutdown();
    _control_plane_socket.shutdown();
  }

}

