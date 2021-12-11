
#include <optional>
#include <sys/wait.h>

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

  void session_shutdown_handler(int, siginfo_t *si, void *)
  {
    spdlog::debug("A session shutdown on PID {}", si->si_pid);

    // Swap the session, delete resources
    Process* pr = Process::get_instance();
    std::thread{ &Process::session_deleted, pr, si->si_pid}.detach();
  }

  void Process::create(
    std::optional<Process> & dest,
    std::string process_id, std::string ip_address, int32_t port,
    std::string hole_puncher_addr, int16_t max_sessions,
    bool verbose, bool enable_swapping
  )
  {
    dest.emplace(
      process_id, hole_puncher_addr,
      max_sessions, verbose
    );

    // Enable swapping first - this can be very time consuming.
    if(enable_swapping && !dest->enable_swapping()) {
      spdlog::error(
        "Failed to initalize session swapping a new process {} with max sessions {}",
        process_id, max_sessions
      );
      dest.reset();
      return;
    }

    // Now connect
    if(!dest->connect(ip_address, port)) {
      dest.reset();
      return;
    }
    return;
  }

  bool Process::connect(std::string ip_address, int32_t port)
  {
    if(_control_plane_socket.connect(sockpp::inet_address(ip_address, port))) {
      spdlog::debug(
        "Succesful connection to control plane {}:{} from {} on process {}.",
        ip_address, port, _control_plane_socket.address().to_string(), _process_id
      );
      return true;
    } else {
      spdlog::error(
        "Unsuccesful connection to control plane {}:{} from {} on process {}.",
        ip_address, port, _control_plane_socket.address().to_string(), _process_id
      );
      return false;
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

    {
      std::lock_guard<std::mutex> guard(_sessions_mutex);
      // Check if we have capacity
      if(_sessions.size() >= static_cast<size_t>(_max_sessions)) {
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

  bool Process::swapping_enabled()
  {
    return _swapper.is_enabled();
  }

  void Process::session_deleted(int pid)
  {
    {
      std::lock_guard<std::mutex> guard(_sessions_mutex);

      // Check if we have the session
      auto it = std::find_if(
        _sessions.begin(), _sessions.end(),
        [pid](const praas::session::SessionFork & s) {
          return s.child_pid == pid;
        }
      );

      if(swapping_enabled()) {
        if(it == _sessions.end()) {
          spdlog::error("Can't swap session with PID {} because it doesn't exist!", pid);
          return;
        }

        spdlog::debug("Swapping session {} to storage.", (*it).session_id);

        // Swap the memory
        _swapper.swap(
          (*it).session_id,
          reinterpret_cast<char*>((*it).memory.ptr()),
          (*it).memory.size()
        );

        spdlog::debug("Swapped session {} to storage, deleting memory.", (*it).session_id);
      }

      // Now clean the object and unlink memory.
      _sessions.erase(it);

      // Finally, wait for PID to clean up zombies
      int status;
      if(waitpid(pid, &status, WNOHANG) != pid) {
        spdlog::error(
          "Couldn't wait for the child {}, error {}.",
          pid, strerror(errno)
        );
      }
    }
  }

}

