
#ifndef __RUNTIME_GLOBAL_MEMORY_HPP__
#define __RUNTIME_GLOBAL_MEMORY_HPP__

#include <optional>

#include <sockpp/tcp_connector.h>
#include <spdlog/spdlog.h>

// FIXME: organize somehow
#include "../local-memory/messages.hpp"

namespace praas::global {

  // FIXME: memory tracking
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

  using praas::local_memory::SessionMessage;

  struct Memory {

    sockpp::tcp_connector* connection;

    bool create(const std::string& id, Object obj)
    {
      SessionMessage msg;
      msg.send(SessionMessage::Type::CREATE, id, obj.size);
      
      connection->write(msg.buffer, msg.BUF_SIZE);
      if(obj.size)
        connection->write(obj.data, obj.size);

      // Wait for result
      connection->read(msg.buffer, msg.BUF_SIZE);
      // No payload on this request
      return !msg.is_error();
    }

    bool erase(const std::string& id)
    {
      return id == "";
    }

    std::optional<Object> get(const std::string& id)
    {
      SessionMessage msg;
      msg.send(SessionMessage::Type::GET, id, 0);
      
      connection->write(msg.buffer, msg.BUF_SIZE);

      // Wait for result
      connection->read(msg.buffer, msg.BUF_SIZE);
      if(msg.is_error())
        return std::optional<Object>{};

      Object obj{new uint8_t[msg.payload()], static_cast<size_t>(msg.payload())};
      connection->read(obj.data, obj.size);
      return obj;
    }

    bool put(const std::string& id, Object)
    {
      return id == "";
    }
  };

}

#endif
