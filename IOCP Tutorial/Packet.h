#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct PacketData
{
	uint32_t DataSize = 0;
	uint32_t SessionIndex = 0;
	char* pPacketData = nullptr;

	void Set(PacketData& value)
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
