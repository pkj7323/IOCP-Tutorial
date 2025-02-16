#pragma once
#include "IOCP.h"

#include "Packet.h"

class EchoServer : public IOCP
{
	bool mIsRunProcessThread = false;

	std::thread mProcessThread;

	std::mutex mLock;
	std::deque<PacketData> mPacketDataQueue;
public:
	EchoServer() = default;
	virtual ~EchoServer() = default;
	virtual void OnConnect(const UINT32& clientIndex) override;
	virtual void OnClose(const UINT32& clientIndex) override;
	virtual void OnReceive(const UINT32& clientIndex, const UINT32& size, char* pData) override;

	void Run(const uint32_t& maxClient);

	void End();

private:
	void processPacket();

	PacketData dequePacketData();


};
