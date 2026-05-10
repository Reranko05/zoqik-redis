#pragma once

#include <WinSock2.h>

#include <string>

bool sendAll(SOCKET clientSocket, const std::string& data);