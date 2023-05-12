#include "StdAfx.h"

export module Route;

export {
  struct RouteResult {
    int status_code = 200;
    std::string content_type { ""s };
    std::vector<char> content { }; 

    RouteResult(int status_code, std::string content_type, std::string content) {
      this->status_code = status_code;
      this->content_type = content_type;
      this->content = std::vector<char>(content.data(), content.data() + content.length());
    }

    RouteResult(int status_code, std::string content_type, std::vector<char> content) {
      this->status_code = status_code;
      this->content_type = content_type;
      this->content = content;
    }
  };

  using route_t = RouteResult(std::string route);
};

export namespace route {
  std::unordered_map<std::string, std::function<route_t>> route_table { };
  
  void register_route(std::string route, route_t route_procedure) {
    route::route_table[route] = route_procedure;
  }
};
