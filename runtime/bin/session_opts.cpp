
#include <cxxopts.hpp>

#include "opts.hpp"

namespace praas::session {

  Options opts(int argc, char ** argv)
  {
    cxxopts::Options options("praas-session", "Handle client connections and allocation of executors.");
    options.add_options()
      ("control-plane-addr", "IP address of control plane.",  cxxopts::value<std::string>())
      ("hole-puncher-addr", "IP address of hole puncher.",  cxxopts::value<std::string>())
      ("session-id", "Session id.",  cxxopts::value<std::string>())
      ("shared-memory-size", "Size of shared memory.", cxxopts::value<int32_t>())
      ("max-functions", "Limit on concurrent functions in session..", cxxopts::value<int32_t>())
      ("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))
    ;
    auto parsed_options = options.parse(argc, argv);

    Options result;
    result.ip_address = parsed_options["control-plane-addr"].as<std::string>();
    result.hole_puncher_address = parsed_options["hole-puncher-addr"].as<std::string>();
    result.session_id = parsed_options["session-id"].as<std::string>();
    result.shared_memory_size = parsed_options["shared-memory-size"].as<int32_t>();
    result.max_functions = parsed_options["max-functions"].as<int32_t>();
    result.verbose = parsed_options["verbose"].as<bool>();

    return result;
  }


}
