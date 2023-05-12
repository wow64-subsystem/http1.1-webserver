#include "StdAfx.h"

export module Client;

#define CLIENT_MTU 1500

export {
  struct ClientExceptionLostConnection : public std::exception {
  private:
    int error_value { 0 };

  public:
    ClientExceptionLostConnection(int error_value, const std::string& message) : std::exception(message.c_str()) {
      this->error_value = error_value;
    }

    ClientExceptionLostConnection(int error_value, const char* message) : std::exception(message) {
      this->error_value = error_value;
    }
  
    int what_error_value() {
      return this->error_value;
    }
  };

  struct ClientExceptionClosedSocket : public std::exception {
    ClientExceptionClosedSocket(const std::string& message) : std::exception(message.c_str()) { }
    ClientExceptionClosedSocket(const char* message) : std::exception(message) { }
  };
};

export class Client {
private:
  SOCKET socket { INVALID_SOCKET };
  std::string_view address { ""sv };

public:
  const int MTU { 1500 };

  Client() = delete;

  Client(SOCKET socket, std::string_view address) {
    this->socket = socket;
    this->address = address;
  }

  ~Client() {
    this->close();
  }

  const void close() {
    shutdown(this->socket, SD_BOTH);
    closesocket(this->socket);
  }

  void send(std::vector<char> block) {
    int block_length = static_cast<int>(block.size());
    
    for (int i = 0; i != block_length;) {
      int block_limit { CLIENT_MTU };

      if (i + block_limit > block_length) {
        block_limit = block_length - i;
      }

      int send_length {
        ::send(this->socket, block.data() + i, block_limit, 0)
      };

      if (send_length == 0) {
        throw ClientExceptionClosedSocket("connection was closed");

        return;
      } else if (send_length < 0) {
        throw ClientExceptionLostConnection(WSAGetLastError(), "exception occured");

        return;
      }

      i += block_limit;
    };
  }

  std::vector<char> read() {
    std::array<char, CLIENT_MTU> stack_block { };

    int received_length {
      recv(this->socket, stack_block.data(), CLIENT_MTU, 0)
    };

    if (received_length == 0) {
      throw ClientExceptionClosedSocket("connection was closed");

      return { };
    } else if (received_length < 0) {
      throw ClientExceptionLostConnection(WSAGetLastError(), "exception occured");

      return { };
    }

    return std::vector<char>(stack_block.data(), stack_block.data() + received_length);
  }
};

#undef CLIENT_MTU
