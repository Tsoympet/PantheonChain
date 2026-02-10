#ifndef CPP_HTTPLIB_STUB_H
#define CPP_HTTPLIB_STUB_H

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace httplib {

struct Request {
    std::string body;
    std::string remote_addr;
};

struct Response {
    int status = 200;
    std::string body;
    std::string content_type;

    void set_content(const std::string& content, const std::string& content_type_in) {
        body = content;
        content_type = content_type_in;
    }
};

class Server {
  public:
    using Handler = std::function<void(const Request&, Response&)>;

    void Post(const std::string& pattern, Handler handler) {
        (void)pattern;
        handlers_.push_back(std::move(handler));
    }

    bool listen(const std::string& host, int port) {
        (void)host;
        (void)port;
        return true;
    }

  private:
    std::vector<Handler> handlers_;
};

}  // namespace httplib

#endif
