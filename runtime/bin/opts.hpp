
#ifndef __RUNTIME_BIN_OPTIONS_HPP__
#define __RUNTIME_BIN_OPTIONS_HPP__

#include <string>

namespace praas::session {

  struct Options {
    std::string ip_address;
    std::string hole_puncher_address;
    std::string session_id;
    int32_t shared_memory_size;
    int32_t max_functions;
    bool verbose;
  };
  Options opts(int argc, char ** argv);

}

#endif
