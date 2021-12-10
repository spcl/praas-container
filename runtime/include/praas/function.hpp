
#ifndef __RUNTIME_FUNCTION_HPP__
#define __RUNTIME_FUNCTION_HPP__

#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

#include <sockpp/tcp_connector.h>

#include <praas/buffer.hpp>

namespace praas::output {
  struct Channel;
}

namespace praas::function {

  struct FunctionsLibrary {
    std::unordered_map<std::string, void*> _libraries;
    std::unordered_map<std::string, void*> _functions;
    static constexpr char DEFAULT_CODE_LOCATION[] = "/code";
    static constexpr char DEFAULT_USER_CONFIG_LOCATION[] = "config.json";
    typedef int (*FuncType)(uint8_t*, uint32_t, praas::output::Channel*);

    FunctionsLibrary();
    ~FunctionsLibrary();
    FuncType get_function(std::string name);
  };

  struct FunctionWorker {
    static constexpr int DEFAULT_BUFFER_SIZE = 1024 * 1024;
    FunctionsLibrary& _library;

    FunctionWorker(FunctionsLibrary &);
    static void invoke(
      std::string fname, ssize_t bytes,
      praas::buffer::Buffer<uint8_t> buf, praas::output::Channel* connection
    );
  private:
    void _invoke(
      std::string fname, ssize_t bytes,
      praas::buffer::Buffer<uint8_t> buf, praas::output::Channel* connection
    );
  };

  struct FunctionWorkers {
    static std::unordered_map<std::thread::id, FunctionWorker*> _workers;
    static FunctionsLibrary* _library;

    static void init(FunctionsLibrary&);
    static void free();
    static FunctionWorker& get(std::thread::id thread_id);
  };

};

#endif
