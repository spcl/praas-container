

#ifndef __RUNTIME_LOCAL_MEMORY_CONNECTIONS_HPP__
#define __RUNTIME_LOCAL_MEMORY_CONNECTIONS_HPP__

#include <cstdint>

#include <sockpp/tcp_socket.h>
#include <spdlog/spdlog.h>

namespace praas::local_memory {

  struct Connection {

    enum class Type {
      SESSION = 0,
      P2P
    };

    sockpp::tcp_socket conn;
    std::atomic<bool> busy;
    int pos;
    std::string id;
    Type type;

    Connection():
      busy(0),
      pos(-1),
      id("")
    {}

    Connection(sockpp::tcp_socket && socket, Type type, int pos):
      busy(0),
      pos(pos),
      id(""),
      type(type)
    {
      conn = std::move(socket);
    }

    Connection(sockpp::tcp_socket && socket, Type type, const std::string& id):
      busy(0),
      pos(-1),
      id(id),
      type(type)
    {
      conn = std::move(socket);
    }

    Connection(Connection && obj)
    {
      conn = std::move(obj.conn);
      pos = std::move(obj.pos);
      type = std::move(obj.type);

      // atomics are not movable
      busy.store(obj.busy.load());
    }

    Connection& operator=(Connection && obj)
    {
      if(this != &obj) {
        conn = std::move(obj.conn);
        pos = std::move(obj.pos);
        type = std::move(obj.type);

        // atomics are not movable
        busy.store(obj.busy.load());
      }

      return *this;
    }
  };

  struct SessionConnections {

    static constexpr int MAX_CONNECTIONS = 64;

    bool active[MAX_CONNECTIONS];
    Connection connections[MAX_CONNECTIONS];

    SessionConnections()
    {
      memset(active, 0, sizeof(bool) * MAX_CONNECTIONS);
    }

    Connection* insert(sockpp::tcp_socket && conn)
    {
      bool* pos = std::find(active, active + MAX_CONNECTIONS, false);
      if(pos == active + MAX_CONNECTIONS) {
        spdlog::error("Can't accept more connections!");
        return nullptr;
      }
      *pos = true;
      int id = std::distance(active, pos);
      connections[id] = Connection{std::move(conn), Connection::Type::SESSION, id};
      return &connections[id];
    }

    void release(int pos)
    {
      active[pos] = false;
      connections[pos].conn.close();
    }

  };

  struct P2PConnections {

    std::unordered_map<std::string, Connection> connections;

    Connection* insert(std::string id, sockpp::tcp_socket && conn)
    {
      Connection& peer = connections[id] = Connection{std::move(conn), Connection::Type::P2P, id};
      return &peer;
    }

    void release(std::string id)
    {
      connections.erase(id);
    }
  };

}

#endif

