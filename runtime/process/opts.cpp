
#include <cxxopts.hpp>

#include "opts.hpp"

namespace praas::process {

  Options opts(int argc, char ** argv)
  {
    cxxopts::Options options("praas-process", "Handle remote memory access.");
    options.add_options()
      ("process-id", "Process ID.",  cxxopts::value<std::string>())
      ("control-plane-address", "IP address of control plane.",  cxxopts::value<std::string>())
      ("hole-puncher-addr", "IP address of hole puncher.",  cxxopts::value<std::string>())
      ("max-sessions", "Max sessions.", cxxopts::value<int>()->default_value("1"))
      ("enable-swapping", "Enable swapping.", cxxopts::value<bool>()->default_value("true"))
      ("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))
    ;
    auto parsed_options = options.parse(argc, argv);

    Options result;
    result.process_id = parsed_options["process-id"].as<std::string>();
    result.control_plane_address = parsed_options["control-plane-address"].as<std::string>();
    result.hole_puncher_address = parsed_options["hole-puncher-addr"].as<std::string>();
    result.max_sessions = parsed_options["max-session"].as<int>();
    result.enable_swapping = parsed_options["enable-swapping"].as<bool>();
    result.verbose = parsed_options["verbose"].as<bool>();

    return result;
  }


}
