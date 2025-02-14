#pragma once
#include "Define.h"

class ClientInfo
{
private:
	int32_t m_nIndex; //클라이언트 인덱스
	SOCKET m_socketClient; //클라이언트와 연결되는 소켓
	OverlappedEx m_RecvOverlappedEx; //클라이언트 소켓의 OverlappedEx IO작업을 위한 변수
	char m_RecvBuf[MAX_SOCKBUF];//데이터 버퍼
public:
	ClientInfo();
	void Init(const UINT32& index);

	void Clear();
	void Close(bool bIsForce_ = false);
	bool BindIOCompletionPort(HANDLE iocpHandle_);
	bool BindRecv();
	bool OnConnect(HANDLE iocpHandle_, SOCKET socket_);

	bool SendMsg(const UINT32& dataSize_, char* pMsg);
	void SendCompleted(const UINT32& dataSize_);

	UINT32 GetIndex() const;

	SOCKET GetSocket() const;

	bool IsConnected() const;

	char* GetRecvBuffer();
	
};
