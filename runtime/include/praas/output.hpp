
#ifndef __RUNTIME_OUTPUT_HPP__
#define __RUNTIME_OUTPUT_HPP__

#include <sockpp/tcp_socket.h>

namespace praas::output {


  struct Channel {

    // Magic number
    enum class Status: int16_t {
      PAYLOAD = 0,
      UNKNOWN_REQUEST = 1,
      OUT_OF_MEMORY = 2
    };

    // Header
    // 2 bytes magic number
    // 2 bytes flag - -1 if not final; otherwise return code from user
    // 4 bytes payload size
    // Total 8 bytes
    static constexpr int HEADER_SIZE = 8;
    char header[HEADER_SIZE];

    sockpp::tcp_socket& socket;

    Channel(sockpp::tcp_socket& socket):
      socket(socket)
    {}

    void send(char* payload, int payload_size, int return_code = -1);
    void mark_end(int return_code);
    void send_error(Status status);
  };

};

#endif
