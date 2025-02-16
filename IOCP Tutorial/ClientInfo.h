#pragma once
#include "Define.h"

//TODO: 5 단계. 1-Send 구현하기
// 버퍼에 쌓아 놓고, send 스레드에서 보내기.
class ClientInfo
{
private:
	int32_t m_nIndex; //클라이언트 인덱스
	SOCKET m_socketClient; //클라이언트와 연결되는 소켓
	OverlappedEx m_RecvOverlappedEx; //클라이언트 소켓의 OverlappedEx IO작업을 위한 변수
	char m_RecvBuf[MAX_SOCKBUF];//데이터 버퍼
public:
	ClientInfo();
	~ClientInfo() = default;
	void Init(const UINT32& index);

	UINT32 GetIndex() const;

	SOCKET GetSocket() const;

	bool IsConnected() const;

	char* GetRecvBuffer();

	bool OnConnect(HANDLE iocpHandle_, SOCKET socket_);
	
	void Close(bool bIsForce_ = false);
	
	bool BindIOCompletionPort(HANDLE iocpHandle_);
	bool BindRecv();
	void Clear();

	bool SendMsg(const UINT32& dataSize_, char* pMsg);
	void SendCompleted(const UINT32& dataSize_);

	
	
};
