#pragma once
#include "IOCP.h"

class ChatServer : public IOCP
{
	//��Ŷ ���� ���� �ؾ���
	std::unique_ptr<PacketManager> m_packetManager;
public:
	ChatServer() = default;
	virtual ~ChatServer() = default;
	virtual void OnConnect(const UINT32& clientIndex) override;
	virtual void OnClose(const UINT32& clientIndex) override;
	virtual void OnReceive(const UINT32& clientIndex, const UINT32& size, char* pData) override;
	void Run(const uint32_t& maxClient);
	void End();
};

