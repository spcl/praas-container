
#ifndef __RUNTIME_PROCESS_HPP__
#define __RUNTIME_PROCESS_HPP__

#include <cstdint>
#include <vector>

#include <sockpp/tcp_acceptor.h>

#include <praas/session.hpp>

namespace praas::process {


  struct Process {
    bool _ending;
    int16_t _max_sessions;
    std::vector<praas::session::Session> _sessions;
    // Connect to process to allocate sessions.
    sockpp::tcp_acceptor _process_socket;

    Process(int32_t port, int16_t max_sessions);
    void start();
    void shutdown();
  };

};

#endif
