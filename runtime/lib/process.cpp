
#include <spdlog/spdlog.h>

#include <praas/process.hpp>

namespace praas::process {

  Process::Process(int32_t port, int16_t max_sessions):
    _ending(false),
    _max_sessions(max_sessions)
  {
    _process_socket.open(port);
  }

  void Process::start()
  {
    if (!_process_socket)
      spdlog::error("Incorrect socket initialization! {}", _process_socket.last_error_str());

    spdlog::info("Process begins work!");
    while(!_ending) {
      sockpp::tcp_socket conn = _process_socket.accept();

      if(_ending)
        break;
      if (!conn)
        spdlog::error("Error accepting incoming connection: {}", _process_socket.last_error_str());

      conn.close();
    }
    spdlog::info("Process ends work!");
  }

  void Process::shutdown()
  {
    spdlog::info("Shutdown process!");
    _ending = true;
    _process_socket.shutdown(); 
  }

}

