
#ifndef __RUNTIME_MESSAGES_HPP__
#define __RUNTIME_MESSAGES_HPP__

#include <memory>
#include <string>
#include <cstring>

namespace praas::messages {

  struct FunctionMessage
  {
    // Header
    // 2 bytes magic number
    // 2 bytes flag - -1 if not final; otherwise return code from user
    // 4 bytes payload size
    // 16 bytes function invocaton id
    // Total 8 bytes
    //
    // Interpretation
    // a) number = 0, flag != -1 -> final message, flag is return code of the function
    // b) number = 0, flag == -1 -> upcoming return data, payload size describe data size
    // c) number > 0 -> error message, number encodes the error from system

    // Magic number
    enum class Status: int16_t {
      PAYLOAD = 0,
      UNKNOWN_REQUEST = 1,
      OUT_OF_MEMORY = 2,
      UNKNOWN_FUNCTION = 3
    };

    static constexpr uint16_t BUF_SIZE = 24;
    int8_t data[BUF_SIZE];

    void fill_payload(int32_t size, std::string function_id)
    {
      *reinterpret_cast<int16_t*>(data) = static_cast<int16_t>(Status::PAYLOAD);
      *reinterpret_cast<int16_t*>(data+2) = -1;
      *reinterpret_cast<int32_t*>(data+4) = size;
      std::strncpy(reinterpret_cast<char*>(data + 8), function_id.data(), 16);
    }

    void fill_error(FunctionMessage::Status status, std::string function_id)
    {
      *reinterpret_cast<int16_t*>(data) = static_cast<int16_t>(status);
      *reinterpret_cast<int16_t*>(data+2) = -1;
      *reinterpret_cast<int32_t*>(data+4) = 0;
      std::strncpy(reinterpret_cast<char*>(data + 8), function_id.data(), 16);
    }

    void fill_end(int16_t return_code, std::string function_id)
    {
      *reinterpret_cast<int16_t*>(data) = static_cast<int16_t>(Status::PAYLOAD);
      *reinterpret_cast<int16_t*>(data+2) = return_code;
      *reinterpret_cast<int32_t*>(data+4) = 0;
      std::strncpy(reinterpret_cast<char*>(data + 8), function_id.data(), 16);
    }
  };

  struct SendMessage
  {
    enum class Type : int16_t {
      PROCESS_IDENTIFICATION = 1,
      SESSION_IDENTIFICATION = 2,
      SESSION_STATUS = 3
    };

    // Process Identification
    // 2 bytes of identifier: 1
    // 16 bytes of process id
    // 18 bytes

    // Session Identification
    // 2 bytes of identifier: 2
    // 16 bytes of session id
    // 18 bytes

    static constexpr uint16_t BUF_SIZE = 24;
    int8_t data[BUF_SIZE];

    SendMessage()
    {
      memset(data, 0, BUF_SIZE);
    }

    void fill_process_identification(std::string process_id);
    void fill_session_identification(std::string session_id);
    void fill_session_status(int32_t status, std::string session_id);
    void fill_session_close(int32_t memory_size, std::string session_id);
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
    // 2 bytes of identifier
    // 4 bytes max functions
    // 4 bytes memory size
    // 16 bytes of session id
    // 26 bytes

    // Function invocation request
    // 2 bytes of identifier
    // 4 bytes payload size
    // 16 bytes of function name
    // 16 bytes of function id
    // 38 bytes

    // FIXME: global memory requests

    // Dictated by the control-plane default packet size of 39 bytes.
    static constexpr uint16_t EXPECTED_LENGTH = 39;
    int8_t data[EXPECTED_LENGTH];

    RecvMessageBuffer()
    {
      memset(data, 0, EXPECTED_LENGTH);
    }

    std::unique_ptr<RecvMessage> parse(ssize_t);
    bool is_session_status();
    bool is_session_closure();
  };

  struct SessionRequestMsg: RecvMessage {
    int8_t* buf;

    SessionRequestMsg(int8_t* buf = nullptr):
      buf(buf)
    {}

    std::string session_id();
    int32_t max_functions();
    int32_t memory_size();
    Type type() const override;
  };

  struct FunctionRequestMsg : RecvMessage {

    int8_t* buf;

    FunctionRequestMsg(int8_t* buf = nullptr):
      buf(buf)
    {}

    std::string function_name();
    std::string function_id();
    int32_t payload();
    Type type() const override;
  };

}

#endif

