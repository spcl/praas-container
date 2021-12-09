
#include <chrono>
#include <stdexcept>
#include <thread>
#include <climits>
#include <sys/time.h>
#include <execinfo.h>

#include <spdlog/spdlog.h>

#include <praas/session.hpp>
#include "opts.hpp"

void handler(int signum)
{
  fprintf(stderr, "Unfortunately, the session has crashed - signal %d.\n", signum);
  void *array[10];
  size_t size;
  // get void*'s for all entries on the stack
  size = backtrace(array, 10);
  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", signum);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

int main(int argc, char ** argv)
{
  auto opts = praas::session::opts(argc, argv);
  if(opts.verbose)
    spdlog::set_level(spdlog::level::debug);
  else
    spdlog::set_level(spdlog::level::info);
  spdlog::set_pattern("[%H:%M:%S:%f] [P %P] [T %t] [%l] %v ");
  spdlog::info("Executing PraaS session executor!");


  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = handler;
  sa.sa_flags   = SA_SIGINFO;

  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGPIPE, &sa, NULL);

  // FIXME: parameters
  praas::session::Session server{opts.session_id, 1, 1};
  server.start(opts.ip_address, opts.hole_puncher_address);

  spdlog::info("Session is closing down");
  return 0;
}

