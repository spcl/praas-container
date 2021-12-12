
#ifndef __RUNTIME_LOCAL_MEMORY_CACHE_HPP__
#define __RUNTIME_LOCAL_MEMORY_CACHE_HPP__

#include <cstdint>
#include <vector>
#include <optional>
#include <string>

#include <sockpp/tcp_connector.h>
#include <tbb/concurrent_hash_map.h>

namespace praas::local_memory {

  struct Object {
    uint8_t* data;
    size_t size;

    Object():
      data(nullptr),
      size(0)
    {}

    template<typename T>
    Object(T* data, size_t size):
      data(reinterpret_cast<uint8_t*>(data)),
      size(size)
    {}

    template<typename T>
    T* ptr() const
    {
      return reinterpret_cast<T*>(data);
    }
  };

  struct Cache {

    typedef oneapi::tbb::concurrent_hash_map<std::string, Object> table_t;
    typedef oneapi::tbb::concurrent_hash_map<std::string, Object>::accessor rw_acc_t;
    typedef oneapi::tbb::concurrent_hash_map<std::string, Object>::const_accessor ro_acc_t;
    table_t _objects_cache;

    bool create(const std::string& id, Object obj);
    bool erase(const std::string& id);
    bool get(const std::string& id, ro_acc_t& accessor);
    bool put(const std::string& id, Object obj);
  };

}

#endif
