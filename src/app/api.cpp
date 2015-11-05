#include "api.hpp"

#include <server_http.hpp>
#include <future>

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

namespace stateline
{
  void ApiResources::set(const std::string& name, json data)
  {
    mutex_.lock();
    resources_[name] = data.dump();
    mutex_.unlock();
  }

  std::string ApiResources::get(const std::string& name)
  {
    mutex_.lock();
    std::string result = resources_[name];
    mutex_.unlock();
    return result;
  }

  void runApiServerThread(HttpServer& server)
  {
    server.start();
  }

  void runApiServer(uint port, ApiResources& res, bool& running)
  {
    HttpServer server(port, 4);

    server.resource["^/test$"]["GET"] = [](HttpServer::Response &resp, std::shared_ptr<HttpServer::Request> request) {
      resp << "HTTP/1.1 200 OK\r\nContent-Length: " << 5 << "\r\n\r\n" << "hello";
    };

    server.resource["^/chains$"]["GET"] =
      [&res](HttpServer::Response &resp, std::shared_ptr<HttpServer::Request> request) {

      std::string content = res.get("chains");
      resp << "HTTP/1.1 200 OK\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
    };

    auto future = std::async(std::launch::async, runApiServerThread, std::ref(server));

    while (running) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    server.stop();
    future.wait();
  }
}
