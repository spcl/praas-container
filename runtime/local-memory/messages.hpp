
#ifndef __RUNTIME_LOCAL_MEMORY_MESSAGES_HPP__
#define __RUNTIME_LOCAL_MEMORY_MESSAGES_HPP__

#include <cstdint>

#include <sockpp/tcp_socket.h>
#include <spdlog/spdlog.h>

namespace praas::local_memory {

  struct SessionMessage {

    enum class Type : int16_t {
      UNKNOWN = -1,
      CREATE = 100,
      GET = 101,
      PUT = 102,
      DELETE = 103,
      CREATE_REPLY = 104,
      GET_REPLY = 105,
      PUT_REPLY = 106,
      DELETE_REPLY = 107,
      REPLY_OFFSET = CREATE_REPLY - CREATE
    };

    static Type reply(Type val)
    {
      return static_cast<Type>(static_cast<int16_t>(val) + static_cast<int16_t>(Type::REPLY_OFFSET));
    }

    // 2 bytes code
    // 4 bytes payload
    // 32 bytes object name
    // for replies, a negative payload indicates a failure
    // 38 bytes
    static constexpr int BUF_SIZE = 38;
    uint8_t buffer[BUF_SIZE];

    void send(Type type, std::string object_id, int32_t payload)
    {
      *reinterpret_cast<int16_t*>(buffer) = static_cast<int16_t>(type);
      *reinterpret_cast<int32_t*>(buffer + 2) = static_cast<int32_t>(payload);
      std::strncpy(reinterpret_cast<char*>(buffer + 6), object_id.data(), 32); 
    }

    Type parse_session_type()
    {
      int16_t type = *reinterpret_cast<int16_t*>(buffer);
      if(type >= static_cast<int16_t>(Type::CREATE)
          && type<= static_cast<int16_t>(Type::DELETE_REPLY)) {
        return static_cast<Type>(type);
      }
      return Type::UNKNOWN;
    }

    int32_t payload()
    {
      return *reinterpret_cast<int32_t*>(buffer + 2);
    }

    bool is_error()
    {
      return payload() < 0;
    }

    std::string object_name()
    {
      size_t len = strnlen(reinterpret_cast<char*>(buffer + 6), 32);
      return std::string{reinterpret_cast<char*>(buffer + 6), len};
    }

  };

  struct RecvBuffer {

    static constexpr int BUF_SIZE = 38;
    uint8_t buffer[BUF_SIZE];

    SessionMessage::Type parse_session_type()
    {
      int16_t type = *reinterpret_cast<int16_t*>(buffer);
      if(type >= static_cast<int16_t>(SessionMessage::Type::CREATE)
          && type<= static_cast<int16_t>(SessionMessage::Type::DELETE_REPLY)) {
        return static_cast<SessionMessage::Type>(type);
      }
      return SessionMessage::Type::UNKNOWN;
    }

    int32_t payload()
    {
      return *reinterpret_cast<int32_t*>(buffer + 2);
    }

    std::string object_name()
    {
      size_t len = strnlen(reinterpret_cast<char*>(buffer + 6), 32);
      return std::string{reinterpret_cast<char*>(buffer + 6), len};
    }
  };

}

#endif

