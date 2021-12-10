
#include "praas/messages.hpp"
#include <fstream>
#include <filesystem>

#include <dlfcn.h>

#include <cJSON.h>
#include <spdlog/spdlog.h>

#include <praas/function.hpp>
#include <praas/output.hpp>
#include <thread>

namespace praas::function {

  FunctionsLibrary::FunctionsLibrary()
  {
    std::stringstream buffer;
    auto path = std::filesystem::path{DEFAULT_CODE_LOCATION};
    {
      auto cfg_path = path / DEFAULT_USER_CONFIG_LOCATION;
      spdlog::debug(
        "Loading function data from {}", cfg_path.string()
      );
      std::ifstream config{cfg_path, std::ios::in};
      buffer << config.rdbuf();
    }
    cJSON * cfg_json = cJSON_Parse(buffer.str().c_str());
    // FIXME: proper error handling
    assert(cfg_json);
    auto functions = cJSON_GetObjectItemCaseSensitive(cfg_json, "functions");
    std::cerr << functions->string << std::endl;
    cJSON* function;
    cJSON_ArrayForEach(function, functions)
    {
      auto key = function->string;
      auto func_name = cJSON_GetObjectItemCaseSensitive(function, "function");
      auto library_name = cJSON_GetObjectItemCaseSensitive(function, "library");
      auto lib_path = path / library_name->valuestring;
      spdlog::debug(
        "Load function {} from {}, store as {}",
        func_name->valuestring, lib_path.string(), key
      );

      auto library = _libraries.find(library_name->valuestring);
      void* library_handle = nullptr;
      if(library == _libraries.end()) {
        library_handle = dlopen(
          lib_path.c_str(),
          RTLD_NOW
        );
        if(!library_handle) {
          spdlog::info("Couldn't open the library, reason: {}", dlerror());
        }
        assert(library_handle);
        _libraries[library_name->valuestring] = library_handle;
      } else
        library_handle = library->second;

      void* func_handle = dlsym(library_handle, func_name->valuestring);
      if(!func_handle)
        spdlog::info("Couldn't get the function, reason: {}", dlerror());
      assert(func_handle);
      _functions[key] = func_handle;
    }
  }

  FunctionsLibrary::~FunctionsLibrary()
  {
    for(auto [k, v] : _libraries)
      dlclose(v);
  }

  FunctionsLibrary::FuncType FunctionsLibrary::get_function(std::string name)
  {
    auto it = _functions.find(name);
    if(it == _functions.end()) {
      return nullptr;
    }
    return reinterpret_cast<FuncType>((*it).second);
  }


  FunctionWorker::FunctionWorker(FunctionsLibrary & library):
    _library(library)
  {}

  void FunctionWorker::invoke(std::string fname, std::string function_id, ssize_t bytes,
    praas::buffer::Buffer<uint8_t> buf, praas::output::Channel* channel
  )
  {
    spdlog::debug("Invoking function {} with {} payload", fname, bytes);
    FunctionWorker & worker = FunctionWorkers::get(std::this_thread::get_id());
    worker._invoke(fname, function_id, bytes, buf, channel);
    spdlog::debug("Invoked function {} with {} payload", fname, bytes);
  }

  void FunctionWorker::_invoke(const std::string& fname, const std::string& function_id, ssize_t bytes,
    praas::buffer::Buffer<uint8_t> buf, praas::output::Channel* channel
  )
  {
    auto func_ptr = _library.get_function(fname);
    if(!func_ptr) {
      channel->send_error(praas::messages::FunctionMessage::Status::UNKNOWN_FUNCTION, function_id);
      spdlog::error("Invoking function {} failed - function unknown!", fname);
      return;
    }
    int return_code = (*func_ptr)(buf.val, bytes, function_id, channel);
    channel->mark_end(return_code, function_id);
  }

  std::unordered_map<std::thread::id, FunctionWorker*> FunctionWorkers::_workers;
  FunctionsLibrary* FunctionWorkers::_library;

  void FunctionWorkers::init(FunctionsLibrary& library)
  {
    FunctionWorkers::_library = &library;
  }

  void FunctionWorkers::free()
  {
    spdlog::info("Closing down function workers.");
    for(auto & v : _workers)
      delete v.second;
  }

  FunctionWorker& FunctionWorkers::get(std::thread::id thread_id)
  {
    auto it = _workers.find(thread_id);
    if(it != _workers.end()) {
      return *(*it).second;
    } else {
      FunctionWorker* ptr = new FunctionWorker(*FunctionWorkers::_library);
      _workers[thread_id] = ptr;
      return *ptr;
    }
  }

}
