#pragma once
#include "pch.h"
#define WIN32_LEAN_AND_MEAN


struct RawPacketData
{
	UINT32 DataSize = 0;
	UINT32 SessionIndex = 0;
	char* pPacketData = nullptr;

	void Set(RawPacketData& value)
	{
		SessionIndex = value.SessionIndex;
		DataSize = value.DataSize;

		pPacketData = new char[value.DataSize];
		CopyMemory(pPacketData, value.pPacketData, value.DataSize);
	}
	void Set(UINT32 sessionIndex, UINT32 dataSize, char* pData)
	{
		SessionIndex = sessionIndex;
		DataSize = dataSize;

		pPacketData = new char[dataSize];
		CopyMemory(pPacketData, pData, dataSize);
	}

	void Release()
	{
		delete pPacketData;
	}
};

struct PacketInfo
{
	UINT32 ClientIndex = 0;
	UINT16 PacketId = 0;
	UINT32 DataSize = 0;
	char* pData = nullptr;
};

enum class PACKET_ID : UINT16
{
	//System
	SYS_USER_CONNECT = 11,
	SYS_USER_DISCONNECT = 12,
	SYS_END = 30,

	//DB
	DB_END = 199,

	//CLIENT
	LOGIN_REQUEST = 201,
	LOGIN_RESPONSE = 202,

	ROOM_ENTER_REQUEST = 206,
	ROOM_ENTER_RESPONSE = 207,

	ROOM_LEAVE_REQUEST = 215,
	ROOM_LEAVE_RESPONSE = 216,

	ROOM_CHAT_REQUEST = 221,
	ROOM_CHAT_RESPONSE = 222,
	ROOM_CHAT_NOTIFY = 223,
};
#pragma pack(push, 1)
struct PACKET_HEADER
{
	UINT16 PacketLength;
	UINT16 PacketId;
	UINT8 Type; // 압축여부 암호여부등 속성을 알아내느 값
};

constexpr UINT32 PACKET_HEADER_SIZE = sizeof(PACKET_HEADER);

constexpr int MAX_USER_ID_LEN = 32;
constexpr int MAX_USER_PW_LEN = 32;

struct LOGIN_REQUEST_PACKET : public PACKET_HEADER
{
	char UserID[MAX_USER_ID_LEN + 1];
	char UserPW[MAX_USER_PW_LEN + 1];
};
constexpr size_t LOGIN_REQUEST_PACKET_SIZE = sizeof(LOGIN_REQUEST_PACKET);

struct LOGIN_RESPONSE_PACKET : public PACKET_HEADER
{
	UINT32 Result;
};

//룸들어가기 요청
struct ROOM_ENTER_REQUEST_PACKET : public PACKET_HEADER
{
	INT32 RoomNumber;
};

struct ROOM_ENTER_RESPONSE_PACKET : public PACKET_HEADER
{
	INT32 Result;
};

//룸나가기 요청
struct ROOM_LEAVE_REQUEST_PACKET : public PACKET_HEADER
{
	INT32 RoomNumber;
};

struct ROOM_LEAVE_RESPONSE_PACKET : public PACKET_HEADER
{
	INT32 Result;
};


//룸채팅 요청
constexpr int MAX_CHAT_MSG_SIZE = 256;
struct ROOM_CHAT_REQUEST_PACKET : public PACKET_HEADER
{
	char Message[MAX_CHAT_MSG_SIZE + 1] = { };
};

struct ROOM_CHAT_RESPONSE_PACKET : public PACKET_HEADER
{
	INT32 Result;
};

struct ROOM_CHAT_NOTIFY_PACKET : public PACKET_HEADER
{
	char UserId[MAX_USER_ID_LEN + 1] = { };
	char Message[MAX_CHAT_MSG_SIZE + 1] = { };
};
#pragma pack(pop)