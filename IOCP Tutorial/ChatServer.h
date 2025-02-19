#pragma once
#include "IOCP.h"
#include "PacketManager.h"

class ChatServer : public IOCP
{
	//패킷 먼저 정의 해야함
	std::unique_ptr<PacketManager> m_packetManager;
public:
	ChatServer() = default;
	virtual ~ChatServer() = default;
	void OnConnect(const UINT32& clientIndex) override;
	void OnClose(const UINT32& clientIndex) override;
	void OnReceive(const UINT32& clientIndex, const UINT32& size, char* pData) override;
	void Run(const uint32_t& maxClient);
	void End();
};

