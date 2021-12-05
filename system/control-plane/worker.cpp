
#include <thread>

#include <redis++.h>

#include "worker.hpp"
#include "resources.hpp"
#include "backend.hpp"

namespace praas::control_plane {

  Worker::Worker(sw::redis::Redis& redis, Resources& resources, backend::Backend& backend):
    generator{rd()},
    uuid_generator{generator},
    redis_conn(redis),
    resources(resources),
    backend(backend)
  {
    payload_buffer_len = 1024;
    payload_buffer.reset(new int8_t[payload_buffer_len]);

  }

  void Worker::resize(ssize_t size)
  {
    if(payload_buffer_len < size || !payload_buffer) {
      payload_buffer_len = size;
      payload_buffer.reset(new int8_t[payload_buffer_len]);
    }
  }

  void Worker::process_client(sockpp::tcp_socket * conn, ClientMessage* msg)
  {
    std::string process_name = msg->process_name();
    std::string function_name = msg->function_name();
    std::string session_id = msg->session_id();
    int32_t payload_size = msg->payload_size();
    spdlog::debug(
      "Request to invoke process {}, function {}, with session {}, payload size {}",
      process_name, function_name, session_id, payload_size
    );

    // Receive user payload in the background
    // Capture the buffer pointer, we're going to reallocate the buffer anyway.
    resize(payload_size);
    auto user_payload  = std::async(
      std::launch::async,
      [this, ptr = payload_buffer.get(), conn]() {
        return conn->read(ptr, payload_buffer_len);
      }
    );

    // Read Redis key - is there session for this?
    std::string redis_key = process_name +  "_" + session_id;
    auto val = redis_conn.get(redis_key);
    // There's a session, redirect!
    if(val) {
      spdlog::info("Redirect invocation to worker");
      // FIXME:
    }
    // Allocate a new process.
    // FIXME: multiple sessions per the same process.
    else {
      spdlog::info("Allocate new process!");
      std::string id = uuids::to_string(uuid_generator());
      redis_conn.set(process_name + "_" + id, "session");

      Process process;
      process.process_id = id;
      process.allocated_sessions = 0;
      PendingAllocation alloc{std::move(payload_buffer), function_name, session_id};
      process.allocations.push_back(std::move(alloc));
      resources.add_process(std::move(process));

      // FIXME: proc name + proc id
      backend.allocate_process("", 1);
    }

  }

  void Worker::process_process(sockpp::tcp_socket * conn, ProcessMessage* msg)
  {
    std::string process_id = msg->process_id();
    spdlog::debug(
      "Received connection from a process {}",
      process_id
    );
    Process* proc = resources.get_process(process_id);
    if(!proc) {
      spdlog::error("Incorrect process id {}!", process_id);
      // FIXME: send error message
      conn->close();
      delete conn;
      return;
    }
    proc->connection = std::move(*conn);

    // Allocate sessions!
    // Add sessions to Redis.

  }

  void Worker::worker(sockpp::tcp_socket * conn)
  {
    auto thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
    spdlog::debug("Worker {} begins processing a request", thread_id);
    Worker& worker = Workers::get(std::this_thread::get_id());

    ssize_t recv_data = conn->read(worker.header.data, Header::BUF_SIZE);
    // EOF
    if(recv_data == 0) {
      spdlog::debug("End of file on connection with {}.", conn->peer_address().to_string());
      conn->close();
      delete conn;
    }
    // Incorrect read
    else if(recv_data == -1) {
      spdlog::debug("Closing connection with {}.", conn->peer_address().to_string());
      conn->close();
      delete conn;
    }
    // Correctly received request
    else {

      std::unique_ptr<MessageType> msg = worker.header.parse(recv_data);
      if(!msg) {
        spdlog::error(
          "Incorrect message of size {} received, closing connection with {}.",
          recv_data, conn->peer_address().to_string()
        );
        conn->close();
        delete conn;
        return;
      }

      if(msg.get()->type() == MessageType::Type::CLIENT) {
        worker.process_client(conn, dynamic_cast<ClientMessage*>(msg.get()));
        conn->close();
        delete conn;
      } else if(msg.get()->type() == MessageType::Type::PROCESS) {
        worker.process_process(conn, dynamic_cast<ProcessMessage*>(msg.get()));
      } else {
        // FIXME: implement handling sessions
        spdlog::error("Unknown type of request!");
        conn->close();
        delete conn;
      }

    }
    spdlog::debug("Worker {} ends processing a request", thread_id);
  }

  std::unordered_map<std::thread::id, Worker*> Workers::_workers;
  sw::redis::Redis* Workers::_redis_conn;
  Resources* Workers::_resources;
  backend::Backend* Workers::_backend;

  void Workers::init(sw::redis::Redis& redis_conn, Resources& resources, backend::Backend& backend)
  {
    Workers::_redis_conn = &redis_conn;
    Workers::_resources = &resources;
    Workers::_backend = &backend;
  }

  void Workers::free()
  {
    for(auto & v : _workers)
      delete v.second;
  }

  Worker& Workers::get(std::thread::id thread_id)
  {
    auto it = _workers.find(thread_id);
    if(it != _workers.end()) {
      return *(*it).second;
    } else {
      Worker* ptr = new Worker(*_redis_conn, *_resources, *_backend);
      _workers[thread_id] = ptr;
      return *ptr;
    }
  }
}