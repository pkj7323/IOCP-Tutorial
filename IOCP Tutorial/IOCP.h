#pragma once
#include "pch.h"
#include "Define.h"
#include "ClientInfo.h"

//TODO: 5 단계. 1-Send 구현하기
// 버퍼에 쌓아 놓고, send 스레드에서 보내기.

class IOCP
{
	SOCKET						m_listenSocket;

	std::vector<ClientInfo*>	m_clientInfos;



	bool						m_isWorkerRun;
	std::vector<std::thread>	m_workerThreads;

	bool						m_isAccepterRun;
	std::thread					m_accepterThread;

	bool						m_isSenderRun;
	std::thread					m_senderThread;

	HANDLE						m_IOCPHandle;

	int							m_clientCnt;
public:
	IOCP();
	virtual ~IOCP();
	bool InitSocket();

	//------서버용 함수-------
	//서버의 주소정보를 소켓과 연결시키고 접속 요청을 받기 위해 
	//소켓을 등록하는 함수
	bool BindandListen(int nBindPort);

	bool StartServer(const UINT32& maxClientCount);

	void DestroyThread();

	ClientInfo* GetClientInfo(const UINT32& sessionIndex);

	bool SendMsg(const UINT32& sessionIndex_, const UINT32& dataSize_, char* pData);

	virtual void OnConnect(const UINT32& clientIndex) {}
	virtual void OnClose(const UINT32& clientIndex) {}
	virtual void OnReceive(const UINT32& clientIndex, const UINT32& size, char* pData) {}
private:
	void createClient(const UINT32& maxClientCount);

	bool createWorkerThread();

	bool createAccepterThread();

	void createSendThread();

	//사용하지 않는 클라이언트 정보 구조체를 반환한다.
	ClientInfo* getEmptyClientInfo();

	void closeSocket(ClientInfo* pClientInfo, bool bIsForce = false);

	void wokerThread();

	void accepterThread();

	void SendThread();
};