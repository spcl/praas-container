
#include <cxxopts.hpp>

#include "opts.hpp"

namespace praas::session {

  Options opts(int argc, char ** argv)
  {
    cxxopts::Options options("praas-session", "Handle client connections and allocation of executors.");
    options.add_options()
      ("a,address", "IP address of control plane.",  cxxopts::value<std::string>())
      ("session-id", "Session id.",  cxxopts::value<std::string>())
      ("shared-memory", "Path to shared memory.", cxxopts::value<std::string>())
      ("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))
    ;
    auto parsed_options = options.parse(argc, argv);

    Options result;
    result.ip_address = parsed_options["address"].as<std::string>();
    result.session_id = parsed_options["session-id"].as<std::string>();
    result.shared_memory = parsed_options["shared-memory"].as<std::string>();
    result.verbose = parsed_options["verbose"].as<bool>();

    return result;
  }


}