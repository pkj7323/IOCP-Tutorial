#include "EchoServer.h"


void EchoServer::OnConnect(const std::uint32_t clientIndex)
{
	std::cout << "[OnConnect] Ŭ���̾�Ʈ ���� : " << clientIndex << std::endl;
}

void EchoServer::OnClose(const std::uint32_t clientIndex)
{
	std::cout << "[OnClose] Ŭ���̾�Ʈ ���� ���� : " << clientIndex << std::endl;
}

void EchoServer::OnReceive(const std::uint32_t clientIndex, const std::uint32_t size, char* pData)
{
	std::cout << "[OnReceive] Ŭ���̾�Ʈ: Index(" << clientIndex << "), dataSize(" << size << ")\n";
}
