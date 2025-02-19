#pragma once
struct PacketInfo;

class User
{
	const UINT32 PACKET_DATA_BUFFER_SIZE = 8096;
public:
	enum class DOMAIN_STATE : UINT16
	{
		NONE = 0,
		LOGIN,
		ROOM,
	};
	User();
	~User();

	void Init(const INT32 index);

	void Clear();

	int SetLogin(char* userId_);

	void EnterRoom(INT32 roomIndex_);

	void SetDomainState(DOMAIN_STATE state_) { m_state = state_; }

	INT32 GetCurrentRoom() const { return m_roomIndex; }

	INT32 GetNetConnIdx() const { return m_index; }

	std::string GetUserID() const { return m_userID; }

	void SetPacketData(const UINT32 dataSize, char* pData);

	PacketInfo GetPacket();



private:

	INT32 m_index;

	INT32 m_roomIndex;

	std::string m_userID;
	bool m_isConfirm;
	std::string m_authToken;

	DOMAIN_STATE m_state;

	UINT32 m_packetDataBufferWPos;
	UINT32 m_packetDataBufferRPos;
	char* m_packetDataBuffer;

};

