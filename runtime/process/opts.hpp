
#ifndef __RUNTIME_BIN_PROCESS_OPTIONS_HPP__
#define __RUNTIME_BIN_PROCESS_OPTIONS_HPP__

#include <string>

namespace praas::process {

  struct Options {
    std::string control_plane_address;
    std::string hole_puncher_address;
    std::string process_id;
    int max_sessions;
    bool enable_swapping;
    bool verbose;
  };
  Options opts(int argc, char ** argv);

}

#endif
