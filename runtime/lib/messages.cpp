
#include <cstring>

#include <praas/messages.hpp>

namespace praas::messages {

  void SendMessage::fill_process_identification(std::string process_id)
  {
    *reinterpret_cast<int16_t*>(data) = static_cast<int16_t>(SendMessage::Type::PROCESS_IDENTIFICATION);
    std::strncpy(reinterpret_cast<char*>(data + 2), process_id.data(), 16);
  }

  void SendMessage::fill_session_identification(std::string session_id)
  {
    *reinterpret_cast<int16_t*>(data) = static_cast<int16_t>(SendMessage::Type::SESSION_IDENTIFICATION);
    std::strncpy(reinterpret_cast<char*>(data + 2), session_id.data(), 16);
  }

  std::unique_ptr<RecvMessage> RecvMessageBuffer::parse(ssize_t data_size)
  {
    if(data_size != RecvMessageBuffer::EXPECTED_LENGTH)
      return nullptr;
    int16_t type = *reinterpret_cast<int16_t*>(data);
    if(type == static_cast<int16_t>(RecvMessage::Type::SESSION_REQUEST)
    ) {
      return std::make_unique<SessionRequestMsg>(data + 2);
    } else if(type == static_cast<int16_t>(RecvMessage::Type::INVOCATION_REQUEST)
    ) {
      return std::make_unique<FunctionRequestMsg>(data + 2);
    } else {
      return nullptr;
    }
  }

  std::string SessionRequestMsg::session_id()
  {
    return std::string{reinterpret_cast<char*>(buf + 8), 16};
  }

  int32_t SessionRequestMsg::max_functions()
  {
    return *reinterpret_cast<int32_t*>(buf);
  }

  int32_t SessionRequestMsg::memory_size()
  {
    return *reinterpret_cast<int32_t*>(buf + 4);
  }

  RecvMessage::Type SessionRequestMsg::type() const
  {
    return RecvMessage::Type::SESSION_REQUEST;
  }

  std::string FunctionRequestMsg::function_name()
  {
    // Avoid direct string construction in case network payload is not a valid null-terminated string
    // strnlen is in POSIX. strnlen_s in C11 but not in C++17
    size_t len = strnlen(reinterpret_cast<char*>(buf + 4), 16);
    return std::string{reinterpret_cast<char*>(buf + 4), len};
  }

  std::string FunctionRequestMsg::function_id()
  {
    // Avoid direct string construction in case network payload is not a valid null-terminated string
    // strnlen is in POSIX. strnlen_s in C11 but not in C++17
    size_t len = strnlen(reinterpret_cast<char*>(buf + 20), 16);
    return std::string{reinterpret_cast<char*>(buf + 20), len};
  }

  int32_t FunctionRequestMsg::payload()
  {
    return *reinterpret_cast<int32_t*>(buf);
  }

  RecvMessage::Type FunctionRequestMsg::type() const
  {
    return RecvMessage::Type::INVOCATION_REQUEST;
  }

}
