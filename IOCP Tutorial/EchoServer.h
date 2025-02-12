#pragma once
#include "IOCP.h"

class EchoServer : public IOCP
{
public:
	void OnConnect(const std::uint32_t clientIndex) override;
	void OnClose(const std::uint32_t clientIndex) override;
	void OnReceive(const std::uint32_t clientIndex, const std::uint32_t size, char* pData) override;
};
