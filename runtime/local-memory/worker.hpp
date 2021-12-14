
#ifndef __RUNTIME_LOCAL_MEMORY_WORKER_HPP__
#define __RUNTIME_LOCAL_MEMORY_WORKER_HPP__

#include <cstdint>

#include <sockpp/tcp_socket.h>
#include <spdlog/spdlog.h>

#include "messages.hpp"
#include "connections.hpp"
#include "cache.hpp"
#include "server.hpp"

namespace praas::local_memory {

  struct Worker {

    static Cache* cache;

    static void handle_session_message(RecvBuffer buffer, Connection* conn, Server* server)
    {
      int32_t payload = buffer.payload(); 
      uint8_t* data = nullptr;
            
      if(payload > 0) { 
        // FIXME: buffer ring
        data = new uint8_t[payload];

        spdlog::debug("Reed payload for {}", buffer.object_name());
        ssize_t recv_data = conn->conn.read_n(data, payload);
        if(recv_data < payload) {
          spdlog::error(
            "Incorrect receive of {} bytes from {}, expected {}",
            recv_data, conn->conn.peer_address().to_string(), payload
          );
          return;
        }
      }
      // Make the connection available for polling again.
      //conn->busy.store(false);
      if(!server->mod_epoll(conn->conn.handle(), conn, EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLONESHOT)) {
        spdlog::error("Couldn't add the process to epoll,closing connection");
        return;
      }

      spdlog::debug("Access for {}", buffer.object_name());
      // FIXME: create should go to global manager
      // FIXME: if get/put/delete fails, we need to go to somewhere else
      bool result = false;
      Cache::ro_acc_t accessor;
      switch(buffer.parse_session_type())
      {
        case SessionMessage::Type::CREATE:
          result = cache->create(buffer.object_name(), Object(data, payload));
          spdlog::debug("Create on object {}, result {}", buffer.object_name(), result);
        break;
        case SessionMessage::Type::GET:
          result = cache->get(buffer.object_name(), accessor);
        break;
        case SessionMessage::Type::PUT:
          result = cache->put(buffer.object_name(), Object(data, payload));
        break;
        case SessionMessage::Type::DELETE:
          result = cache->erase(buffer.object_name());
        break;
        default:
          spdlog::error("Unknown type of operation: {}!", buffer.parse_session_type());
      }

      SessionMessage reply;
      int32_t ret_payload;
      if(result)
        ret_payload = accessor.empty() ? 0 : accessor->second.size;
      else
        ret_payload = -1;
      spdlog::debug("Return reply for object {}, payload size {}", buffer.object_name(), ret_payload);
      reply.send(SessionMessage::reply(buffer.parse_session_type()), buffer.object_name(), ret_payload);

      conn->conn.write(reply.buffer, reply.BUF_SIZE);
      if(ret_payload > 0)
        conn->conn.write(accessor->second.data, ret_payload);

      spdlog::debug("Done {}", buffer.object_name());
      accessor.release();
      spdlog::debug("Released {}", buffer.object_name());
      //delete[] data;
    }

    static void handle_message(RecvBuffer buffer, Connection* conn, Server* server)
    {
      if(conn->type == Connection::Type::SESSION) {
        handle_session_message(std::move(buffer), conn, server);
      }
    }

  };

}

#endif

