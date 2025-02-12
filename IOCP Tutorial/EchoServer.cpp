#include "EchoServer.h"


void EchoServer::OnConnect(const std::uint32_t clientIndex)
{
	std::cout << "[OnConnect] 클라이언트 접속 : " << clientIndex << std::endl;
}

void EchoServer::OnClose(const std::uint32_t clientIndex)
{
	std::cout << "[OnClose] 클라이언트 접속 종료 : " << clientIndex << std::endl;
}

void EchoServer::OnReceive(const std::uint32_t clientIndex, const std::uint32_t size, char* pData)
{
	std::cout << "[OnReceive] 클라이언트: Index(" << clientIndex << "), dataSize(" << size << ")\n";
}
