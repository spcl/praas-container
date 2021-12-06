
#include <chrono>
#include <stdexcept>
#include <thread>
#include <climits>
#include <sys/time.h>

#include <spdlog/spdlog.h>

#include <praas/session.hpp>
#include "opts.hpp"

int main(int argc, char ** argv)
{
  auto opts = praas::session::opts(argc, argv);
  if(opts.verbose)
    spdlog::set_level(spdlog::level::debug);
  else
    spdlog::set_level(spdlog::level::info);
  spdlog::set_pattern("[%H:%M:%S:%f] [P %P] [T %t] [%l] %v ");
  spdlog::info("Executing PraaS session executor!");

  // FIXME: parameters
  praas::session::Session server{opts.session_id, 1, 1};
  server.start(opts.ip_address);

  spdlog::info("Control plane is closing down");
  return 0;
}

