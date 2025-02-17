#pragma once

#include "Define.h"
//6 단계. 1 - Send 구현하기
//queue에 담아 놓고 순차적으로 보내기.
//버퍼로하면 빠르지만 데이터 낭비 발생
class ClientInfo
{
	INT32			m_nIndex; //클라이언트 인덱스
	SOCKET			m_socketClient; //클라이언트와 연결되는 소켓
	OverlappedEx	m_RecvOverlappedEx; //Recv overalappedEx I/O 작업을 위한 변수

	char			m_RecvBuf[MAX_SOCKBUF];//데이터 버퍼

	std::mutex		m_Lock;
	std::queue<std::unique_ptr<OverlappedEx>> m_sendDataQ;

public:
	ClientInfo();
	~ClientInfo() = default;
	void Init(const UINT32& index);

	UINT32 GetIndex() const { return m_nIndex; }

	SOCKET GetSocket() const { return m_socketClient; }

	bool IsConnected() const { return m_socketClient != INVALID_SOCKET; }

	char* GetRecvBuffer() { return m_RecvBuf; }


	bool OnConnect(HANDLE iocpHandle_, SOCKET socket_);
	
	void Close(bool bIsForce_ = false);

	void Clear() { };
	
	bool BindIOCompletionPort(HANDLE iocpHandle_);

	bool BindRecv();

	// 1개의 스레드에서만 호출해야한다.
	bool SendMsg(const UINT32& dataSize_, char* pMsg);

	bool SendIO();

	void SendCompleted(const UINT32& dataSize_);

	
	
};
