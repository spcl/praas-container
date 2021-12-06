
#include <praas/function.hpp>>

namespace praas::function {

  FunctionsLibrary::FunctionsLibrary(std::string user_configuration)
  {
    // FIXME
  }

  FunctionsLibrary::~FunctionsLibrary()
  {
    // FIXME
  }

  void FunctionsLibrary::invoke(std::string name)
  {
    // FIXME
  }

  FunctionWorker::FunctionWorker(FunctionsLibrary & library):
    _library(library)
  {
    resize(DEFAULT_BUFFER_SIZE);
  }

  void FunctionWorker::resize(ssize_t size)
  {
    if(payload_buffer_len < size || !payload_buffer) {
      payload_buffer_len = size;
      payload_buffer.reset(new int8_t[payload_buffer_len]);
    }
  }

  void FunctionWorkers::init(FunctionsLibrary& library)
  {
    FunctionWorkers::_library = library;
  }

  void FunctionWorkers::free()
  {
    for(auto & v : _workers)
      delete v.second;
  }

  FunctionWorker& FunctionWorkers::get(std::thread::id thread_id)
  {
    auto it = _workers.find(thread_id);
    if(it != _workers.end()) {
      return *(*it).second;
    } else {
      FunctionWorker* ptr = new FunctionWorker(FunctionWorkers::_library);
      _workers[thread_id] = ptr;
      return *ptr;
    }
  }

}
