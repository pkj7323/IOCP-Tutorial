#pragma once
#include "Define.h"
#pragma comment(lib, "mswsock.lib")
//6 단계. 1 - Send 구현하기
//queue에 담아 놓고 순차적으로 보내기.
//버퍼로하면 빠르지만 데이터 낭비 발생
class ClientInfo
{
	INT32			m_nIndex; //클라이언트 인덱스
	HANDLE			m_hIOCP; //IOCP 핸들

	INT64			m_isConnect;
	UINT64			m_lastestClosedTimeSec;

	SOCKET			m_socketClient; //클라이언트와 연결되는 소켓

	OverlappedEx	m_RecvOverlappedEx; //Recv overalappedEx I/O 작업을 위한 변수
	char			m_RecvBuf[MAX_SOCKBUF];//데이터 버퍼

	OverlappedEx	m_AcceptContext;
	char			m_acceptBuf[64];


	std::mutex		m_sendLock;
	std::queue<std::unique_ptr<OverlappedEx>> m_sendDataQ;

public:
	ClientInfo(const UINT32& index, HANDLE iocpHandle_);
	~ClientInfo() = default;
	void Init(const UINT32& index, HANDLE iocpHandle_);

	UINT32 GetIndex() const { return m_nIndex; }

	bool IsConnected() const { return m_socketClient != INVALID_SOCKET; }

	SOCKET GetSocket() const { return m_socketClient; }

	UINT64 GetLastestClosedTimeSec() const { return m_lastestClosedTimeSec; }

	char* GetRecvBuffer() { return m_RecvBuf; }


	bool OnConnect(HANDLE iocpHandle_, SOCKET socket_);
	
	void Close(bool bIsForce_ = false);

	void Clear() { }

	bool PostAccept(SOCKET listenSocket_, UINT64 curTimeSec_);

	bool AcceptCompletion();
	
	bool BindIOCompletionPort(HANDLE iocpHandle_);

	bool BindRecv();

	bool SendMsg(const UINT32& dataSize_, char* pMsg);

	void SendCompleted(const UINT32& dataSize_);
private:
	bool sendIO();

	bool setSocketOption();

	
	
};
