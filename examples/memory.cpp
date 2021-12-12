
#include "../runtime/include/praas/global_memory.hpp"

#include <sockpp/inet_address.h>

int main()
{
  sockpp::tcp_connector conn;
  conn.connect(sockpp::inet_address("127.0.0.1", 7000));
  praas::global::Memory memory{&conn};

  char buf[] = "MY SECRET";
  spdlog::info("Send create");
  bool result = memory.create("/obj1", praas::global::Object{buf, strlen(buf) + 1});
  spdlog::info("Send create, result {}", result);

  auto obj = memory.get("/obj1");
  spdlog::info("Send get, expected result {}", obj.value().ptr<char>());

  auto obj2 = memory.get("/obj2");
  spdlog::info("Send get, expected result false: {}", obj2.has_value());

  conn.close();
  return 0;
}
