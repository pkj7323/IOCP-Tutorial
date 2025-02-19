#pragma once
#include "Packet.h"


class UserManager;
class RoomManager;
class RedisManager;

class PacketManager
{
	typedef void (PacketManager::* PROCESS_RECV_PACKET_FUNCTION)(UINT32, UINT16, char*);
	std::unordered_map<int, PROCESS_RECV_PACKET_FUNCTION> m_recvFunctionMap;

	UserManager* m_userManager;

	std::function<void(int, char*)> m_sendMQDataFunc;

	bool m_isRunProcessThread = false;

	std::thread m_processThread;

	std::mutex m_lock;

	std::deque<UINT32> m_inComingPacketUserIndex;

	std::deque<PacketInfo> m_systemPacketQueue;

public:
	PacketManager() = default;
	~PacketManager() = default;
	void Init(const UINT32& maxClient_);

	bool Run();

	void End();

	void ReceivePacketData(const UINT32& clientIndex, const UINT32& size, char* pData);

	void PushSystemPacket(PacketInfo& packetInfo);

	std::function<void(UINT32,UINT32,char*)> SendPacketFunc;
private:

	void CreateCompent(const UINT32 maxClient_);

	void ClearConnectionInfo(INT32 clientIndex_);

	void EnqueuePacketData(const UINT32 clientIndex_);
	PacketInfo DequePacketData();

	PacketInfo DequeSystemPacketData();


	void ProcessPacket();

	void ProcessRecvPacket(const UINT32 clientIndex_, const UINT16 packetId_, const UINT16 packetSize_, char* pPacket_);

	void ProcessUserConnect(UINT32 clientIndex_, UINT16 packetSize_, char* pPacket_);
	void ProcessUserDisConnect(UINT32 clientIndex_, UINT16 packetSize_, char* pPacket_);

	void ProcessLogin(UINT32 clientIndex_, UINT16 packetSize_, char* pPacket_);

};
