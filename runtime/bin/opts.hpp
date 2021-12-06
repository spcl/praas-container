
#ifndef __RUNTIME_BIN_OPTIONS_HPP__
#define __RUNTIME_BIN_OPTIONS_HPP__

#include <string>

namespace praas::session {

  struct Options {
    std::string ip_address;
    std::string session_id;
    std::string shared_memory;
    bool verbose;
  };
  Options opts(int argc, char ** argv);

}

#endif
