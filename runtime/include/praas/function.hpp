
#ifndef __RUNTIME_FUNCTION_HPP__
#define __RUNTIME_FUNCTION_HPP__

#include <memory>
#include <string>
#include <thread>
#include <unordered_map>


namespace praas::function {

  struct FunctionsLibrary {
    void* _handle;
    std::unordered_map<std::string, void*> _functions;

    FunctionsLibrary(std::string user_configuration);
    ~FunctionsLibrary();
    void invoke(std::string name);
  };

  struct FunctionWorker {
    static constexpr int DEFAULT_BUFFER_SIZE = 1024 * 1024;
    std::unique_ptr<int8_t[]> payload_buffer;
    ssize_t payload_buffer_len;
    FunctionsLibrary& _library;

    FunctionWorker(FunctionsLibrary &);
    void resize(ssize_t size);
  };

  struct FunctionWorkers {
    static std::unordered_map<std::thread::id, FunctionWorker*> _workers;
    static FunctionsLibrary& _library;

    static void init(FunctionsLibrary&);
    static void free();
    static FunctionWorker& get(std::thread::id thread_id);
  };

};

#endif
