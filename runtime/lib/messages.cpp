
#include <cstring>

#include <praas/messages.hpp>

namespace praas::messages {

  ssize_t SendMessage::fill_process_identification(std::string process_id)
  {
    *reinterpret_cast<int16_t*>(data) = static_cast<int16_t>(SendMessage::Type::PROCESS_IDENTIFICATION);
    std::strncpy(reinterpret_cast<char*>(data + 2), process_id.data(), 16);
    return 18;
  }

  ssize_t SendMessage::fill_session_identification(std::string session_id)
  {
    *reinterpret_cast<int16_t*>(data) = static_cast<int16_t>(SendMessage::Type::SESSION_IDENTIFICATION);
    std::strncpy(reinterpret_cast<char*>(data + 2), session_id.data(), 16);
    return 18;
  }

  std::unique_ptr<RecvMessage> RecvMessage::parse(ssize_t data_size)
  {
    int16_t type = *reinterpret_cast<int16_t*>(data);
    if(data_size == SessionRequestMsg::EXPECTED_LENGTH && type == 0) {
      return std::make_unique<SessionRequestMsg>(data + 2);
    } else {
      return nullptr;
    }
  }

  std::string SessionRequestMsg::session_id()
  {
    return std::string{reinterpret_cast<char*>(buf + 6), 15};
  }

  int32_t SessionRequestMsg::max_functions()
  {
    return *reinterpret_cast<int32_t*>(buf);

  }

  int32_t SessionRequestMsg::memory_size()
  {

  }

  RecvMessage::Type SessionRequestMsg::type() const
  {
    return RecvMessage::Type::SESSION_REQUEST;
  }
}