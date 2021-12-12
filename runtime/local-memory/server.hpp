
#ifndef __RUNTIME_LOCAL_MEMORY_SERVER_HPP__
#define __RUNTIME_LOCAL_MEMORY_SERVER_HPP__

#include <cstdint>
#include <vector>
#include <optional>
#include <sys/epoll.h>

#include <sockpp/tcp_connector.h>
#include <sockpp/tcp_acceptor.h>
#include <thread_pool.hpp>
#include <spdlog/spdlog.h>

#include "cache.hpp"
#include "connections.hpp"

namespace praas::local_memory {

  struct Server {

    bool _ending;
    int _port;
    std::string _hole_puncher_address;
    std::string _gm_ip_address;
    // Connect to process to allocate sessions.
    sockpp::tcp_connector _global_memory_controller;
    // object cache
    Cache _cache;
    // thread pool
    thread_pool _pool;
    // active connections
    SessionConnections _session_conns;
    P2PConnections _peer_conns;

    static constexpr int MAX_EPOLL_EVENTS = 32;
    int _epoll_fd;

    Server(
      int threads, int port, std::string gm_ip_address, std::string hole_puncher_address
    ):
      _ending(false),
      _port(port),
      _hole_puncher_address(hole_puncher_address),
      _gm_ip_address(gm_ip_address),
      _pool(threads)
    {}

    template<typename T>
    bool add_epoll(int handle, T* data, uint32_t epoll_events)
    {
      spdlog::debug(
        "Adding to epoll connection, fd {}, ptr {}, events {}",
        handle,
        fmt::ptr(static_cast<void*>(data)),
        epoll_events
      );

      epoll_event event;
      memset(&event, 0, sizeof(epoll_event));
      event.events  = epoll_events;
      event.data.ptr = static_cast<void*>(data);

      if(epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, handle, &event) == -1) {
        spdlog::error("Adding socket to epoll failed, reason: {}", strerror(errno));
        return false;
      }
      return true;
    }

    void start();
    void shutdown();
  };

}

#endif
