
#include "cache.hpp"

namespace praas::local_memory {


  bool Cache::create(const std::string& key, Object obj)
  {
    rw_acc_t accessor;
    bool ret = _objects_cache.insert(accessor, key);
    accessor->second = obj;
    return ret;
  }

  bool Cache::put(const std::string& key, Object obj)
  {
    rw_acc_t accessor;
    bool ret = _objects_cache.find(accessor, key);
    if(ret)
      accessor->second = obj;
    return ret;
  }

  bool Cache::get(const std::string& key, ro_acc_t& accessor)
  {
    return _objects_cache.find(accessor, key);
  }

  bool Cache::erase(const std::string& key)
  {
    return _objects_cache.erase(key);

  }

}
