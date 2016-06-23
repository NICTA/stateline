#include "api.hpp"

//#include <server_http.hpp>
#include <future>
#include <fstream>
//#include <boost/filesystem.hpp>

//using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
//namespace fs = boost::filesystem;

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
    auto result = resources_[name];
    mutex_.unlock();
    return result;
  }

  std::string ApiResources::getAll()
  {
    mutex_.lock();
    auto result = resources_;
    mutex_.unlock();

    std::string str = "{";
    for (auto it = result.begin(); it != result.end(); ++it) {
      if (it != result.begin()) str += ",";
      str += "\"" + it->first + "\": " + it->second;
    }
    return str + "}";
  }

  /*
  void runApiServerThread(HttpServer& server)
  {
    server.start();
  }*/

  void runApiServer(uint port, ApiResources& res, bool& running)
  {
    /* TODO: use graphite
    HttpServer server(port, 4);

    server.resource["^/test$"]["GET"] = [](HttpServer::Response &resp, std::shared_ptr<HttpServer::Request> request)
    {
      resp << "HTTP/1.1 200 OK\r\nContent-Length: " << 5 << "\r\n\r\n" << "hello";
    };

    server.resource["^/chains$"]["GET"] =
      [&res](HttpServer::Response &resp, std::shared_ptr<HttpServer::Request> request)
    {
      std::string content = res.get("chains");
      resp << "HTTP/1.1 200 OK\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
    };

    server.resource["^/all$"]["GET"] =
      [&res](HttpServer::Response &resp, std::shared_ptr<HttpServer::Request> request)
    {
      std::string content = res.getAll();
      resp << "HTTP/1.1 200 OK\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
    };

    // Adapted from https://github.com/eidheim/Simple-Web-Server/blob/master/http_examples.cpp
    server.default_resource["GET"] =
      [](HttpServer::Response &resp, std::shared_ptr<HttpServer::Request> request)
    {
      fs::path webRoot("frontend");
      if (!fs::exists(webRoot))
      {
        std::string content = "Server is misconfigured.";
        resp << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
        return;
      }

      auto path = webRoot;
      path += request->path;
      if (fs::is_directory(path))
        path += "index.html";

      if (fs::exists(path) && fs::is_regular_file(path))
      {
        std::ifstream ifs(path.string(), std::ifstream::in | std::ifstream::binary);
        if (!ifs)
        {
          std::string content = "Not found.";
          resp << "HTTP/1.1 404 Not Found\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
          return;
        }

        // From the simple web example https://github.com/eidheim/Simple-Web-Server/blob/master/http_examples.cpp
        ifs.seekg(0, std::ios::end);
        size_t length = ifs.tellg();

        ifs.seekg(0, std::ios::beg);

        resp << "HTTP/1.1 200 OK\r\nContent-Length: " << length << "\r\n\r\n";

        // Read and send 128 KB at a time
        uint buffer_size=131072;
        std::vector<char> buffer;
        buffer.reserve(buffer_size);
        uint read_length;
        try
        {
          while((read_length=ifs.read(&buffer[0], buffer_size).gcount())>0)
          {
            resp.write(&buffer[0], read_length);
            resp.flush();
          }
        }
        catch(const std::exception &e)
        {
          std::cerr << "Connection interrupted, closing file" << std::endl;
        }

        ifs.close();
      }
      else
      {
        std::string content = "Not found.";
        resp << "HTTP/1.1 404 Not Found\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
      }
    };

    auto future = std::async(std::launch::async, runApiServerThread, std::ref(server));

    while (running)
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    server.stop();
    future.wait();*/
  }
}
