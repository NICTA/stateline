#include <server_http.hpp>
#include <future>

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

namespace stateline
{
  void runApiServerThread(HttpServer& server)
  {
    server.start();
  }

  void runApiServer(uint port, bool& running)
  {
    HttpServer server(port, 4);
    server.resource["^/test$"]["GET"] = [](HttpServer::Response &resp, std::shared_ptr<HttpServer::Request> request) {
      resp << "HTTP/1.1 200 OK\r\nContent-Length: " << 5 << "\r\n\r\n" << "hello";
    };

    auto future = std::async(std::launch::async, runApiServerThread, std::ref(server));

    while (running) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    server.stop();
    future.wait();
  }
}
