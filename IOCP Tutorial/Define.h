#pragma once
#include <winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "ws2_32")
#include <iostream>


#include <thread>
#include <vector>
#include <array>
#include <windows.h>
#include <string>

constexpr std::uint32_t MAX_SOCKBUF = 256;
constexpr std::uint32_t MAX_SOCK_SENDBUF = 4096;
constexpr std::uint32_t MAX_WORKERTHREAD = 4;


enum class IOOperation
{
	recv,
	send
};

//overlapped I/O를 위해 구조체를 확장시킨다.
struct OverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;
	SOCKET        m_socketClient;
	WSABUF        m_wsaBuf;
	IOOperation   m_eOperation;
};

#include "ClientInfo.h"
