#include "pch.h"
#include "User.h"
#include "UserManager.h"
#include "PacketManager.h"

#include "ErrorCode.h"

void PacketManager::Init(const UINT32& maxClient_)
{
	m_recvFunctionMap = std::unordered_map<int, PROCESS_RECV_PACKET_FUNCTION>();

	m_recvFunctionMap[static_cast<int>(PACKET_ID::SYS_USER_CONNECT)] = &PacketManager::ProcessUserConnect;
	m_recvFunctionMap[static_cast<int>(PACKET_ID::SYS_USER_DISCONNECT)] = &PacketManager::ProcessUserDisConnect;

	m_recvFunctionMap[static_cast<int>(PACKET_ID::LOGIN_REQUEST)] = &PacketManager::ProcessLogin;

	CreateCompent(maxClient_);
}

bool PacketManager::Run()
{
	//이부분을 패킷 처리 부분으로 이동시킨다.
	m_isRunProcessThread = true;
	m_processThread = std::thread([this]() { ProcessPacket(); });

	return true;
}

void PacketManager::End()
{
	m_isRunProcessThread = false;
	if (m_processThread.joinable())
	{
		m_processThread.join();
	}
}

void PacketManager::ReceivePacketData(const UINT32& clientIndex, const UINT32& size, char* pData)
{
	auto user = m_userManager->GetUserByConnIdx(clientIndex);
	user->SetPacketData(size, pData);

	EnqueuePacketData(clientIndex);
}

void PacketManager::PushSystemPacket(PacketInfo& packetInfo)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_systemPacketQueue.push_back(packetInfo);
}

void PacketManager::CreateCompent(const UINT32 maxClient_)
{
	m_userManager = new UserManager();
	m_userManager->Init(maxClient_);
}

void PacketManager::ClearConnectionInfo(INT32 clientIndex_)
{
}

void PacketManager::EnqueuePacketData(const UINT32 clientIndex_)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_inComingPacketUserIndex.push_back(clientIndex_);
}

PacketInfo PacketManager::DequePacketData()
{
	UINT32 userIndex = 0;
	{
		std::lock_guard<std::mutex> lock(m_lock);
		if (m_inComingPacketUserIndex.empty())
		{
			return PacketInfo();
		}
		else
		{
			userIndex = m_inComingPacketUserIndex.front();
			m_inComingPacketUserIndex.pop_front();
		}
		
	}

	auto pUser = m_userManager->GetUserByConnIdx(userIndex);
	auto packetData = pUser->GetPacket();

	packetData.ClientIndex = userIndex;
	return packetData;
}

PacketInfo PacketManager::DequeSystemPacketData()
{
	std::lock_guard<std::mutex> lock(m_lock);
	if (m_systemPacketQueue.empty())
	{
		return PacketInfo();
	}
	auto packetData = m_systemPacketQueue.front();
	m_systemPacketQueue.pop_front();

	return packetData;
}

void PacketManager::ProcessPacket()
{
	while (m_isRunProcessThread)
	{
		bool isIdle = true;

		if (auto packetData = DequePacketData(); packetData.PacketId > static_cast<UINT16>(PACKET_ID::SYS_END))
		{
			isIdle = false;
			ProcessRecvPacket(packetData.ClientIndex, packetData.PacketId, packetData.DataSize, packetData.pData);
		}

		if (auto packetData = DequeSystemPacketData(); packetData.PacketId != 0)
		{
			isIdle = false;
			ProcessRecvPacket(packetData.ClientIndex, packetData.PacketId, packetData.DataSize, packetData.pData);
		}

		if (isIdle)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

void PacketManager::ProcessRecvPacket(const UINT32 clientIndex_, const UINT16 packetId_, const UINT16 packetSize_,
	char* pPacket_)
{
	auto iter = m_recvFunctionMap.find(packetId_);
	if (iter != m_recvFunctionMap.end())
	{
		(this->*iter->second)(clientIndex_, packetSize_, pPacket_);
	}
}

void PacketManager::ProcessUserConnect(UINT32 clientIndex_, UINT16 packetSize_, char* pPacket_)
{
	std::cout << "[ProcessUserConnect] clientIndex: "<<clientIndex_ << std::endl;
	auto pUser = m_userManager->GetUserByConnIdx(clientIndex_);
	pUser->Clear();
}

void PacketManager::ProcessUserDisConnect(UINT32 clientIndex_, UINT16 packetSize_, char* pPacket_)
{
	std::cout << "[ProcessUserDisConnect] clientIndex: " << clientIndex_ << std::endl;
	ClearConnectionInfo(clientIndex_);
}

void PacketManager::ProcessLogin(UINT32 clientIndex_, UINT16 packetSize_, char* pPacket_)
{
	if (LOGIN_REQUEST_PACKET_SIZE != packetSize_)
	{
		std::cerr << "[에러] 로그인 패킷 사이즈가 맞지 않습니다.\n";
		return;
	}

	auto pLoginReqPacket = reinterpret_cast<LOGIN_REQUEST_PACKET*>(pPacket_);

	auto pUser = pLoginReqPacket->UserID;
	std::cout << "request user id = " << pUser << std::endl;

	LOGIN_RESPONSE_PACKET loginResPacket;
	loginResPacket.PacketId = static_cast<UINT16>(PACKET_ID::LOGIN_RESPONSE);
	loginResPacket.PacketLength = sizeof(LOGIN_RESPONSE_PACKET);

	if (m_userManager->GetCurrentUserCnt() >= m_userManager->GetMaxUserCnt())
	{
		//접속자수가 최대수를 차지해서 접속불가
		loginResPacket.Result = static_cast<UINT16>(ERROR_CODE::LOGIN_USER_USED_ALL_OBJ);
		SendPacketFunc(clientIndex_, sizeof(LOGIN_RESPONSE_PACKET), reinterpret_cast<char*>(&loginResPacket));
		return;
	}

	//여기에서 이미 접속된 유저인지 확인하고, 접속된 유저라면 실패한다.
	if (m_userManager->FindUserIndexByID(pUser) == -1)
	{
		loginResPacket.Result = static_cast<UINT16>(ERROR_CODE::NONE);
		SendPacketFunc(clientIndex_, sizeof(LOGIN_RESPONSE_PACKET), reinterpret_cast<char*>(&loginResPacket));
	}
	else
	{
		//접속중인 유저여서 실패를 반환한다.
		loginResPacket.Result = static_cast<UINT16>(ERROR_CODE::LOGIN_USER_ALREADY);
		SendPacketFunc(clientIndex_, sizeof(LOGIN_RESPONSE_PACKET), reinterpret_cast<char*>(&loginResPacket));
		return;
	}
}
