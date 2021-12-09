
#include <praas/output.hpp>

namespace praas::output {

  void Channel::send(char* payload, int payload_size, int return_code)
  {
    *reinterpret_cast<int16_t*>(header) = static_cast<int16_t>(Status::PAYLOAD);
    *reinterpret_cast<int16_t*>(header+2) = return_code;
    *reinterpret_cast<int32_t*>(header+4) = payload_size;
    socket.write(header, HEADER_SIZE);
    socket.write(payload, payload_size);
  }

  void Channel::mark_end(int return_code)
  {
    *reinterpret_cast<int16_t*>(header) = static_cast<int16_t>(Status::PAYLOAD);
    *reinterpret_cast<int16_t*>(header+2) = return_code;
    *reinterpret_cast<int32_t*>(header+4) = 0;
    socket.write(header, HEADER_SIZE);
  }

  void Channel::send_error(Status status)
  {
    *reinterpret_cast<int16_t*>(header) = static_cast<int16_t>(status);
    *reinterpret_cast<int16_t*>(header+2) = -1;
    *reinterpret_cast<int32_t*>(header+4) = 0;
    socket.write(header, HEADER_SIZE);
  }

};
