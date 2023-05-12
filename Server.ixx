#include "StdAfx.h"

export module Server;

import Client;
import Route;

export using server_accept_t = bool(SOCKET socket, std::string_view address);

void client_thread_procedure(SOCKET socket, std::string_view address) {
  struct HttpGetParameter {
    std::string request { };
    std::string location { };
    std::string protocol { };

    HttpGetParameter(std::string line) {
      enum State {
        StateRequest,
        StateLocation,
        StateProtocol,
      };

      std::vector<char> request_vec { };
      std::vector<char> location_vec { };
      std::vector<char> protocol_vec { };
      State state { StateRequest };

      for (auto v : line) {
        bool end_of_string { false };

        switch (state) {
        case StateRequest: {
          if (v == ' ') {
            state = StateLocation;
          } else {
            request_vec.push_back(v);
          }

          break;
        }
        case StateLocation: {
          if (v == ' ') {
            state = StateProtocol;
          } else {
            location_vec.push_back(v);
          }

          break;
        }
        case StateProtocol: {
          if (v == '\r' || v == '\n') {
            end_of_string = true;

            break;
          } else {
            protocol_vec.push_back(v);
          }

          break;
        }
        default:
          break;
        }

        if (end_of_string) {
          break;
        }
      }

      this->request = std::string(request_vec.data(), request_vec.size());
      this->location = std::string(location_vec.data(), location_vec.size());
      this->protocol = std::string(protocol_vec.data(), protocol_vec.size());
    }
  };

  auto SendStatusCode = [](Client* client, uint32_t status_code, std::string content_type = ""s, std::vector<char> content_data = { }) -> void {
    bool has_content = false;
    std::vector<char> block { };
    std::stringstream ss(""s);

    ss << "HTTP/1.1 " << status_code << std::endl;
    ss << "Server: V2-WS1.1" << std::endl;

    if (content_type != "" && content_data.size()) {
      ss << "Content-Length: " << content_data.size() << std::endl;
      ss << "Content-Type: " << content_type << std::endl;
      ss << std::endl;

      has_content = true;
    }

    std::string builded_string { ss.str() };

    for (auto v : builded_string) {
      block.push_back(v);
    }

    if (has_content) {
      for (auto v : content_data) {
        block.push_back(v);
      }
    }

    client->send(block);
  };

  std::shared_ptr<Client> client {
    std::make_shared<Client>(socket, address)
  };

  bool keep_alive_bool = false;

  do {
    try {
      std::vector<char> block {
        client->read()
      };

      std::string block_text { std::string(block.data(), block.size()) };
      std::string first_line { ""s };
      std::stringstream ss(block_text);
      std::getline(ss, first_line, '\r');

      std::cout << "> "s << block_text << std::endl;

      auto [ client_request, client_location, client_protocol ] = HttpGetParameter(first_line);

      if (client_protocol == "HTTP/1.1"s) {
        if (client_request == "GET"s) {
          if (route::route_table[client_location]) {
            auto [ route_status_code, content_type, content_data ] = route::route_table[client_location](client_location);

            SendStatusCode(client.get(), route_status_code, content_type, content_data);

            //TODO: if it throws an exception send error code
          } else {
            SendStatusCode(client.get(), 404); // Not Found
          }
        } else {
          SendStatusCode(client.get(), 501); // Not Implemented
        }
      } else {
        SendStatusCode(client.get(), 505); // HTTP Version Not Supported
      }

    } catch (ClientExceptionLostConnection& e) {
      std::cout << "connection is closed with client " << address << " due an error: " << e.what_error_value() << std::endl;
    } catch (ClientExceptionClosedSocket& e) {
      UNREFERENCED_PARAMETER(e);

      std::cout << "connection is closed with client " << address << std::endl;
    } catch (std::exception& e) {
      std::cout << "exception occured in client thread procedure: client " << address << ", error: " << e.what() << std::endl;

      break;
    }
  } while (keep_alive_bool);
}

export class Server {
private:
  SOCKET listening_socket { INVALID_SOCKET };
  std::atomic_bool running_sate { false };

public:
  Server() { }

  ~Server() {
    this->close();
  }

  void close() {
    this->running_sate.store(false);

    if (this->listening_socket != INVALID_SOCKET) {
      shutdown(this->listening_socket, SD_BOTH);
      closesocket(this->listening_socket);
      this->listening_socket = INVALID_SOCKET;
    }
  }

  std::optional<error_t> create(const std::string_view& listening_ip, int16_t service_port) {
    sockaddr_in socket_address {
      .sin_family = AF_INET,
      .sin_port = htons(service_port),
    };

    if (inet_pton(AF_INET, listening_ip.data(), &socket_address.sin_addr) <= 0) {
      return WSAGetLastError();
    }

    this->listening_socket = {
      socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
    };

    if (this->listening_socket == INVALID_SOCKET) {
      return WSAGetLastError();
    }

    int32_t boolean_true { true };

    if (setsockopt(this->listening_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&boolean_true), sizeof(boolean_true))) {
      error_t error { WSAGetLastError() };

      shutdown(this->listening_socket, SD_BOTH);
      closesocket(this->listening_socket);
      this->listening_socket = INVALID_SOCKET;

      return error;
    }

    if (bind(this->listening_socket, reinterpret_cast<sockaddr*>(&socket_address), sizeof(socket_address))) {
      error_t error { WSAGetLastError() };

      shutdown(this->listening_socket, SD_BOTH);
      closesocket(this->listening_socket);
      this->listening_socket = INVALID_SOCKET;

      return error;
    }

    return std::nullopt;
  }

  std::optional<error_t> listen(std::function<server_accept_t> accept_function) {
    std::optional<error_t> return_value { std::nullopt };

    if (!accept_function) {
      throw std::invalid_argument("accept_function is nullptr");
    }

    if (::listen(this->listening_socket, SOMAXCONN)) {
      return_value = WSAGetLastError();

      return return_value;
    }

    this->running_sate.store(true);

    while (this->running_sate.load()) {
      sockaddr_in address { };
      int32_t address_length { sizeof(address) };
      SOCKET client_socket { accept(this->listening_socket, reinterpret_cast<sockaddr*>(&address), &address_length) };

      if (client_socket == SOCKET_ERROR) {
        return_value = WSAGetLastError();

        break;
      }

      std::array<char, 32> buffer { };
      auto client_ip_friendly { std::string(inet_ntop(AF_INET, &address.sin_addr, buffer.data(), buffer.size())) };

      if (!accept_function(client_socket, client_ip_friendly)) {
        continue;
      }

      std::thread client_thread(client_thread_procedure, client_socket, client_ip_friendly);
      client_thread.detach();
    }

    this->running_sate.store(false);

    return return_value;
  }
};
