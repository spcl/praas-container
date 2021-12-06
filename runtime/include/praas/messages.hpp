
#ifndef __RUNTIME_MESSAGES_HPP__
#define __RUNTIME_MESSAGES_HPP__

#include <memory>
#include <string>

namespace praas::messages {

  struct SendMessage
  {
    enum class Type : int16_t {
      PROCESS_IDENTIFICATION = 1,
      SESSION_IDENTIFICATION = 2
    };

    // Process Identification
    // 2 bytes of identifier: 1
    // 16 bytes of process id
    // 18 bytes

    // Session Identification
    // 2 bytes of identifier: 2
    // 16 bytes of session id
    // 18 bytes

    static constexpr uint16_t BUF_SIZE = 18;
    int8_t data[BUF_SIZE];

    ssize_t fill_process_identification(std::string process_id);
    ssize_t fill_session_identification(std::string session_id);
  };

  struct RecvMessage {
    enum class Type : int16_t {
      SESSION_REQUEST = 12,
      INVOCATION_REQUEST = 13
    };

    virtual Type type() const = 0;
  };

  struct RecvMessageBuffer
  {

    // Session request
    // 2 bytes of identifier: 1
    // 4 bytes max functions
    // 4 bytes memory size
    // 16 bytes of session id
    // 26 bytes

    // Function request
    // FIXME

    static constexpr uint16_t REQUEST_BUF_SIZE = 26;
    int8_t data[REQUEST_BUF_SIZE];

    std::unique_ptr<RecvMessage> parse(ssize_t);
  };

  struct SessionRequestMsg: RecvMessage {
    static constexpr uint16_t EXPECTED_LENGTH = 26;
    int8_t* buf;

    SessionRequestMsg(int8_t* buf = nullptr):
      buf(buf)
    {}

    std::string session_id();
    int32_t max_functions();
    int32_t memory_size();
    Type type() const override;
  };

}

#endif

