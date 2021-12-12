
#ifndef __RUNTIME_BIN_LOCAL_MEMORY_OPTIONS_HPP__
#define __RUNTIME_BIN_LOCAL_MEMORY_OPTIONS_HPP__

#include <string>

namespace praas::local_memory {

  struct Options {
    std::string gm_ip_address;
    std::string hole_puncher_address;
    int threads;
    int port;
    bool verbose;
  };
  Options opts(int argc, char ** argv);

}

#endif
