#pragma once
#include "IOCP.h"
#include <deque>
#include <mutex>
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
	virtual void OnConnect(const std::uint32_t clientIndex) override;
	virtual void OnClose(const std::uint32_t clientIndex) override;
	virtual void OnReceive(const std::uint32_t clientIndex, const std::uint32_t size, char* pData) override;

	void Run(const uint32_t& maxClient);

	void End();

private:
	void processPacket();

	PacketData dequePacketData();


};
