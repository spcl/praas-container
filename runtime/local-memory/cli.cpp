
#include <chrono>
#include <climits>
#include <sys/time.h>
#include <execinfo.h>

#include <spdlog/spdlog.h>

#include "server.hpp"
#include "opts.hpp"

void handler(int signum)
{
  fprintf(stderr, "Unfortunately, the local memory controller has crashed - signal %d.\n", signum);
  void *array[10];
  size_t size;
  // get void*'s for all entries on the stack
  size = backtrace(array, 10);
  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", signum);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

praas::local_memory::Server * instance = nullptr;

void signal_handler(int)
{
  if(instance)
    instance->shutdown();
}

int main(int argc, char ** argv)
{
  auto opts = praas::local_memory::opts(argc, argv);
  if(opts.verbose)
    spdlog::set_level(spdlog::level::debug);
  else
    spdlog::set_level(spdlog::level::info);
  spdlog::set_pattern("[%H:%M:%S:%f] [P %P] [T %t] [%l] %v ");
  spdlog::info("Executing PraaS local memory controller!");

  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = handler;
  sa.sa_flags   = SA_SIGINFO;

  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGPIPE, &sa, NULL);

  // Catch SIGINT
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = &signal_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, NULL);

  praas::local_memory::Server server(opts.threads, opts.port, opts.gm_ip_address, opts.hole_puncher_address);
  instance = &server;
  server.start();

  spdlog::info("Session is closing down");
  return 0;
}

