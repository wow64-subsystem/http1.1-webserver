#include "StdAfx.h"

import Server;
import Client;
import Route;

static bool accept_procedure(SOCKET socket, std::string_view address) {
  std::cout << "client accepted, " << address << std::endl;

  return true;
}

static std::optional<error_t> initialize_windows_sockets() {
  WSADATA WsaData { };

  if (WSAStartup(MAKEWORD(2, 2), &WsaData)) {
    return static_cast<error_t>(WSAGetLastError());
  }

  return std::nullopt;
}

static void uninitialize_windows_sockets() {
  WSACleanup();
}

static void signal(int signal) {
  if (signal == SIGINT) {
    std::cout << "caught Ctrl+C, exiting...";
  }
}

static RouteResult route_root(std::string route) {
  return RouteResult(200, "text/html"s, "deez nuts"s);
}

int main() {
  std::cout << "started" << std::endl;
  std::signal(SIGINT, signal);

  route::register_route("/"s, route_root);

  while (true) {
    auto result { initialize_windows_sockets() };

    if (result.has_value()) {
      if (result.value() == WSASYSNOTREADY) {
        std::cout << "network is not ready yet, wariting..." << std::endl;
        std::this_thread::sleep_for(1s);
  
        continue;
      }

      std::cout << "WSAStartup rised an error code: 0x" << std::hex << result.value() << std::endl;

      return 0;
    }

    break;
  }

  const std::string all_network_adapters { "0.0.0.0"s };

  while (true) {
    try {    
      Server server;

      auto result { server.create(all_network_adapters, 8080) };

      if (result.has_value()) {
        std::cout << "failed to create the server, error: " << result.value() << std::endl;
      } else {
        auto result {
          server.listen(accept_procedure)
        };

        server.close();

        if (result.has_value()) {
          std::cout << "server failed to listen for incoming connections, error: " << result.value() << std::endl;

          continue;
        }
      }
    } catch (std::exception& e) {
      std::cout << "exception caught in the mail loop, error: " << e.what() << std::endl;

      break;
    }
    
    std::this_thread::sleep_for(1s);
  }

  uninitialize_windows_sockets();

  return 0;
}
