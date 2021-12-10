
#ifndef __RUNTIME_OUTPUT_HPP__
#define __RUNTIME_OUTPUT_HPP__

#include <sockpp/tcp_socket.h>

#include <spdlog/spdlog.h>

#include <praas/messages.hpp>

namespace praas::output {


  struct Channel {

    sockpp::tcp_socket& socket;

    Channel(sockpp::tcp_socket& socket):
      socket(socket)
    {}

    bool send(char* payload, int payload_size)
    {
      praas::messages::FunctionMessage msg;
      msg.fill_payload(payload_size);
      ssize_t bytes = ::send(socket.handle(), msg.data, msg.BUF_SIZE, MSG_NOSIGNAL);
      if(bytes == -1)  {
        spdlog::error("Sending payload header failed! Reason: {}", strerror(errno));
        return false;
      }
      if(payload_size) {
        bytes = ::send(socket.handle(), payload, payload_size, MSG_NOSIGNAL);
        if(bytes == -1)  {
          spdlog::error("Sending payload failed! Reason: {}", strerror(errno));
          return false;
        } else {
          spdlog::debug("Sent {} bytes of payload!", payload_size);
          return true;
        }
      }
      return true;
    }

    bool mark_end(int return_code)
    {
      praas::messages::FunctionMessage msg;
      msg.fill_end(return_code);
      ssize_t bytes = ::send(socket.handle(), msg.data, msg.BUF_SIZE, MSG_NOSIGNAL);
      if(bytes == -1)  {
        spdlog::error("Sending end mark failed! Reason: {}", strerror(errno));
        return false;
      } else {
        spdlog::debug("Sent end mark!");
        return true;
      }
    }

    bool send_error(praas::messages::FunctionMessage::Status status)
    {
      praas::messages::FunctionMessage msg;
      msg.fill_error(status);
      ssize_t bytes = ::send(socket.handle(), msg.data, msg.BUF_SIZE, MSG_NOSIGNAL);
      if(bytes == -1)  {
        spdlog::error("Error notification failed! Reason: {}", strerror(errno));
        return false;
      } else {
        spdlog::debug("Sent error!");
        return true;
      }
    }
  };

};

#endif
