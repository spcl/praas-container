
#ifndef __RUNTIME_OUTPUT_HPP__
#define __RUNTIME_OUTPUT_HPP__

#include <sockpp/tcp_socket.h>

#include <spdlog/spdlog.h>

namespace praas::output {


  struct Channel {

    // Magic number
    enum class Status: int16_t {
      PAYLOAD = 0,
      UNKNOWN_REQUEST = 1,
      OUT_OF_MEMORY = 2,
      UNKNOWN_FUNCTION = 3
    };

    // Header
    // 2 bytes magic number
    // 2 bytes flag - -1 if not final; otherwise return code from user
    // 4 bytes payload size
    // Total 8 bytes
    static constexpr int HEADER_SIZE = 8;

    sockpp::tcp_socket& socket;

    Channel(sockpp::tcp_socket& socket):
      socket(socket)
    {}

    void send(char* payload, int payload_size)
    {
      char header[HEADER_SIZE];
      *reinterpret_cast<int16_t*>(header) = static_cast<int16_t>(Status::PAYLOAD);
      *reinterpret_cast<int16_t*>(header+2) = -1;
      *reinterpret_cast<int32_t*>(header+4) = payload_size;
      ssize_t bytes = socket.write(header, HEADER_SIZE);
      if(bytes == -1)  {
        spdlog::error("Sending payload header failed! Reason: {}", strerror(errno));
        return;
      }
      bytes = socket.write(payload, payload_size);
      if(bytes == -1)  {
        spdlog::error("Sending payload failed! Reason: {}", strerror(errno));
        return;
      } else {
        spdlog::debug("Sent {} bytes of payload!", payload_size);
      }
    }

    void mark_end(int return_code)
    {
      char header[HEADER_SIZE];
      *reinterpret_cast<int16_t*>(header) = static_cast<int16_t>(Status::PAYLOAD);
      *reinterpret_cast<int16_t*>(header+2) = return_code;
      *reinterpret_cast<int32_t*>(header+4) = 0;
      ssize_t bytes = socket.write(header, HEADER_SIZE);
      if(bytes == -1)  {
        spdlog::error("Sending end mark failed! Reason: {}", strerror(errno));
        return;
      } else {
        spdlog::debug("Sent end mark!");
      }
    }

    void send_error(Status status)
    {
      char header[HEADER_SIZE];
      *reinterpret_cast<int16_t*>(header) = static_cast<int16_t>(status);
      *reinterpret_cast<int16_t*>(header+2) = -1;
      *reinterpret_cast<int32_t*>(header+4) = 0;
      ssize_t bytes = socket.write(header, HEADER_SIZE);
      if(bytes == -1)  {
        spdlog::error("Error notification failed! Reason: {}", strerror(errno));
        return;
      } else {
        spdlog::debug("Sent error!");
      }
    }
  };

};

#endif
