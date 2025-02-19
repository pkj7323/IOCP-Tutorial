#include "pch.h"
#include "User.h"

#include "Packet.h"
User::User() : m_index{ -1 }, m_roomIndex{ -1 }, m_userID{}, m_isConfirm{ false }, m_authToken{},
m_state{ DOMAIN_STATE::NONE }, m_packetDataBufferWPos{ 0 }, m_packetDataBufferRPos{ 0 },
m_packetDataBuffer{ nullptr }
{
}


User::~User()
{
	if (m_packetDataBuffer != nullptr)
	{
		delete[] m_packetDataBuffer;
	}
}

void User::Init(const INT32 index)
{
	m_index = index;
	m_packetDataBuffer = new char[PACKET_DATA_BUFFER_SIZE];
}

void User::Clear()
{
	
	m_roomIndex = -1;
	m_userID.clear();
	m_isConfirm = false;
	
	m_state = DOMAIN_STATE::NONE;

	m_packetDataBufferWPos = 0;
	m_packetDataBufferRPos = 0;
}

int User::SetLogin(char* userId_)
{
	m_state = DOMAIN_STATE::LOGIN;
	m_userID = userId_;

	return 0;
}

void User::EnterRoom(INT32 roomIndex_)
{
	m_roomIndex = roomIndex_;
	m_state = DOMAIN_STATE::ROOM;
}

void User::SetPacketData(const UINT32 dataSize, char* pData)
{
	if (m_packetDataBufferWPos + dataSize >= PACKET_DATA_BUFFER_SIZE)
	{
		auto remainDataSize = m_packetDataBufferWPos - m_packetDataBufferRPos;
		if (remainDataSize > 0)
		{
			CopyMemory(&m_packetDataBuffer[0], &m_packetDataBuffer[m_packetDataBufferRPos], remainDataSize);
			m_packetDataBufferWPos = remainDataSize;
		}
		else
		{
			m_packetDataBufferWPos = 0;
		}
		m_packetDataBufferRPos = 0;
	}
	CopyMemory(&m_packetDataBuffer[m_packetDataBufferWPos], pData, dataSize);
	m_packetDataBufferWPos += dataSize;
}

PacketInfo User::GetPacket()
{
	const int PACKET_SIZE_LENGTH = 2;
	const int PACKET_TYPE_LENGTH = 2;
	short packetSize = 0;

	UINT32 remainByte = m_packetDataBufferWPos - m_packetDataBufferRPos;

	if (remainByte < PACKET_HEADER_SIZE)
	{
		return PacketInfo();
	}

	auto pHeader = reinterpret_cast<PACKET_HEADER*>(&m_packetDataBuffer[m_packetDataBufferRPos]);

	if (pHeader->PacketLength > remainByte)
	{
		return PacketInfo();
	}

	PacketInfo packetInfo;
	packetInfo.PacketId = pHeader->PacketId;
	packetInfo.DataSize = pHeader->PacketLength;
	packetInfo.pData = &m_packetDataBuffer[m_packetDataBufferRPos];

	m_packetDataBuffer += pHeader->PacketLength;

	return packetInfo;
}
