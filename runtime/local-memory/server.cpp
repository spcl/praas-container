
#include <cstring>

#include <sockpp/tcp_acceptor.h>
#include <sockpp/tcp_connector.h>
#include <spdlog/spdlog.h>

#include "server.hpp"
#include "cache.hpp"
#include "messages.hpp"
#include "worker.hpp"

namespace praas::local_memory {

  void Server::start()
  {
    sockpp::tcp_acceptor listen;
    listen.open(_port);
    if (!listen) {
      spdlog::error("Incorrect socket initialization! {}", listen.last_error_str());
      return;
    }

    Worker::cache = &_cache;

    _epoll_fd = epoll_create(255);
    if(_epoll_fd < 0) {
      spdlog::error("Incorrect epoll initialization! {}", strerror(errno));
      return;
    }
    if(!add_epoll(listen.handle(), &listen, EPOLLIN | EPOLLPRI))
      return;

    epoll_event events[MAX_EPOLL_EVENTS];
    while(!_ending) {

      int events_count = epoll_wait(_epoll_fd, events, MAX_EPOLL_EVENTS, -1);

      // Finish if we failed (but we were not interrupted), or when end was requested.
      if(_ending || (events_count == -1 && errno != EINVAL))
        break;

      for(int i = 0; i < events_count; ++i) {

        // Accept connection
        if(events[i].data.ptr == static_cast<void*>(&listen)) {

          sockpp::tcp_socket conn = listen.accept();
          // FIXME: disable until we figure out what to do with epoll
          //conn.read_timeout(std::chrono::microseconds(_read_timeout * 1000));
          if(conn.is_open())
            spdlog::debug("Accepted new connection from {}.", conn.peer_address().to_string());

          if (!conn) {
            spdlog::error("Error accepting incoming connection: {}", listen.last_error_str());
            continue;
          }

          // This should be a session - peers we accept somewhere else
          Connection* ptr = _session_conns.insert(std::move(conn));
          add_epoll(ptr->conn.handle(), ptr, EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLONESHOT);

        }
        // New message from a process. 
        else {
          Connection* conn_obj = static_cast<Connection*>(events[i].data.ptr);
          if(conn_obj->busy.load()) {
            spdlog::debug("busy, skip");
            continue;
          }
          sockpp::tcp_socket* conn = &conn_obj->conn;

          spdlog::debug("Polling new data from {}", conn->peer_address().to_string());
          RecvBuffer msg;

          ssize_t recv_data = conn->read_n(msg.buffer, msg.BUF_SIZE);

          if(recv_data == 0) {
            spdlog::debug("End of file on connection with {}.", conn->peer_address().to_string());
            if(conn_obj->type == Connection::Type::SESSION)
              _session_conns.release(conn_obj->pos);
            else
              _peer_conns.release(conn_obj->id);
            continue;
          }
          // Incorrect read
          else if(recv_data == -1) {
            spdlog::debug(
              "Closing connection with {}. Reason: {}", conn->peer_address().to_string(),
              strerror(errno)
            );
            if(conn_obj->type == Connection::Type::SESSION)
              _session_conns.release(conn_obj->pos);
            else
              _peer_conns.release(conn_obj->id);
            continue;
          }

          // Store process sending the message in user-provided data
          //_pool.push_task(Worker::handle_message, msg, conn_obj);
          std::thread{Worker::handle_message, msg, conn_obj, this}.detach();
          //conn_obj->busy.store(true);

        }
      }
    }
  }

  void Server::shutdown()
  {
    spdlog::info("Shutdown process!");
    _ending = true;
  }
}
