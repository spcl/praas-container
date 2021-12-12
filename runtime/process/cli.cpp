
#include <chrono>
#include <stdexcept>
#include <thread>
#include <climits>
#include <charconv>

#include <signal.h>
#include <sys/time.h>
#include <execinfo.h>

#include <spdlog/spdlog.h>

#include <praas/process.hpp>
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

std::tuple<std::string_view, std::string_view> split(std::string & str, char split_character)
{
  auto pos = str.find(split_character);
  if(pos == std::string::npos)
    return std::make_tuple("", "");
  else
    return std::make_tuple(
      std::string_view(str.data(), pos),
      std::string_view(str.data() + pos + 1, str.length() - pos - 1)
    );
}

praas::process::Process* instance = nullptr;

void signal_handler(int)
{
  assert(instance);
  instance->shutdown();
}

int main(int argc, char ** argv)
{
  auto opts = praas::process::opts(argc, argv);
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

  // Catch SIGINT
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = &signal_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, NULL);

  std::optional<praas::process::Process> process;
  auto [ip_address, port_str] = split(opts.control_plane_address, ':');
  int port;
  // https://stackoverflow.com/questions/56634507/safely-convert-stdstring-view-to-int-like-stoi-or-atoi
  std::from_chars(port_str.data(), port_str.data() + port_str.size(), port);
  praas::process::Process::create(
    process, opts.process_id, std::string{ip_address}, port,
    opts.hole_puncher_address, opts.max_sessions,
    opts.verbose, opts.enable_swapping
  );
  if(!process.has_value())
    return 1;
  instance = &process.value();
  process.value().start();

  spdlog::info("Session is closing down");
  return 0;
}

