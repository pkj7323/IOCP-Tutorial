#include "pch.h"
#include "EchoServer.h"


void EchoServer::OnConnect(const UINT32& clientIndex)
{
	std::cout << "[OnConnect] 클라이언트 접속 : " << clientIndex << std::endl;
}

void EchoServer::OnClose(const UINT32& clientIndex)
{
	std::cout << "[OnClose] 클라이언트 접속 종료 : " << clientIndex << std::endl;
}

void EchoServer::OnReceive(const UINT32& clientIndex, const UINT32& size, char* pData)
{
	std::cout << "[OnReceive] 클라이언트: Index(" << clientIndex << "), dataSize(" << size << ")\n";

	RawPacketData packet;
	packet.Set(clientIndex, size, pData);

	std::lock_guard<std::mutex> lock(mLock);
	mPacketDataQueue.push_back(packet);
}

void EchoServer::Run(const uint32_t& maxClient)
{
	mIsRunProcessThread = true;
	mProcessThread = std::thread([this](){ processPacket(); });

	StartServer(maxClient);
}

void EchoServer::End()
{
	mIsRunProcessThread = false;
	if (mProcessThread.joinable())
	{
		mProcessThread.join();
	}

	DestroyThread();
}

void EchoServer::processPacket()
{
	while (mIsRunProcessThread)
	{
		auto packetData = dequePacketData();
		if (packetData.DataSize != 0)
		{
			SendMsg(packetData.SessionIndex, packetData.DataSize, packetData.pPacketData);
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

RawPacketData EchoServer::dequePacketData()
{
	RawPacketData packetData;

	std::lock_guard<std::mutex> lock(mLock);
	if (mPacketDataQueue.empty())
	{
		return RawPacketData();
	}

	packetData.Set(mPacketDataQueue.front());

	mPacketDataQueue.front().Release();
	mPacketDataQueue.pop_front();

	return packetData;
}
