
#ifndef __LOCAL_WORKER_SERVER_HPP__
#define __LOCAL_WORKER_SERVER_HPP__

#include <thread>
#include <optional>

#include <sockpp/tcp_acceptor.h>

namespace praas::process {
  struct Process;
}

namespace praas::local_worker {

  struct Options {
    std::string hole_puncher_address;
    int processes;
    int port;
    bool verbose;
    bool enable_swapping;
  };
  Options opts(int, char**);

  struct Server
  {
    std::optional<praas::process::Process>* _processes;
    std::thread** _threads;
    std::string _hole_puncher_address;
    int _max_processes;
    int _port;
    bool _ending;
    bool _verbose;
    bool _enable_swapping;
    sockpp::tcp_acceptor _listen;

    Server(Options &);
    ~Server();

    void start();
    void shutdown();
  };

}

#endif

