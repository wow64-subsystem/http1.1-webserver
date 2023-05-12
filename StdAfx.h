#pragma once

#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <iostream>
#include <thread>
#include <chrono>
#include <tuple>
#include <string>
#include <string_view>
#include <optional>
#include <atomic>
#include <array>
#include <functional>
#include <span>
#include <sstream>
#include <map>

#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace std::chrono_literals;

using error_t = int;

#pragma comment(lib, "ws2_32.lib")
