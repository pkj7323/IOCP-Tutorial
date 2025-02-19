#include "pch.h"
#include "ChatServer.h"

void ChatServer::OnConnect(const UINT32& clientIndex)
{
	std::cout << "[OnConnect] 클라이언트 접속 : " << clientIndex << std::endl;
	PacketInfo packet{ clientIndex, static_cast<UINT16>(PACKET_ID::SYS_USER_CONNECT), 0 };
	m_packetManager->PushSystemPacket(packet);
}

void ChatServer::OnClose(const UINT32& clientIndex)
{
	std::cout << "[OnClose] 클라이언트 접속 종료 : " << clientIndex << std::endl;
	PacketInfo packet{ clientIndex, static_cast<UINT16>(PACKET_ID::SYS_USER_DISCONNECT), 0 };
	m_packetManager->PushSystemPacket(packet);
}

void ChatServer::OnReceive(const UINT32& clientIndex, const UINT32& size, char* pData)
{
	std::cout << "[OnReceive] 클라이언트: Index(" << clientIndex << "), dataSize(" << size << ")\n";
	m_packetManager->ReceivePacketData(clientIndex, size, pData);
}

void ChatServer::Run(const uint32_t& maxClient)
{
	auto sendPacketFunc = [&](UINT32 clientIndex, UINT16 packetSize, char* pSendPacket)
		{
			SendMsg(clientIndex, packetSize, pSendPacket);
		};

	m_packetManager = std::make_unique<PacketManager>();
	m_packetManager->SendPacketFunc = sendPacketFunc;
	m_packetManager->Init(maxClient);
	m_packetManager->Run();

	StartServer(maxClient);
}

void ChatServer::End()
{
	m_packetManager->End();

	DestroyThread();
}
