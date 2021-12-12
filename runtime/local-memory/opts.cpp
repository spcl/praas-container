
#include <cxxopts.hpp>

#include "opts.hpp"

namespace praas::local_memory {

  Options opts(int argc, char ** argv)
  {
    cxxopts::Options options("praas-local-memory-controller", "Handle remote memory access.");
    options.add_options()
      ("global-memory-addr", "IP address of global memory controllerr.",  cxxopts::value<std::string>())
      ("hole-puncher-addr", "IP address of hole puncher.",  cxxopts::value<std::string>())
      ("port", "Port to listen on.", cxxopts::value<int>()->default_value("7000"))
      ("threads", "Handler threads.", cxxopts::value<int>()->default_value("1"))
      ("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))
    ;
    auto parsed_options = options.parse(argc, argv);

    Options result;
    result.gm_ip_address = parsed_options["global-memory-addr"].as<std::string>();
    result.hole_puncher_address = parsed_options["hole-puncher-addr"].as<std::string>();
    result.port = parsed_options["port"].as<int>();
    result.threads = parsed_options["threads"].as<int>();
    result.verbose = parsed_options["verbose"].as<bool>();

    return result;
  }


}
