#pragma once
#include "Define.h"
#include "ClientInfo.h"

//TODO: 4 단계. 네트워크와 로직(패킷 or 요청) 처리 각각의 스레드로 분리하기
//Send를 Recv와 다른 스레드에서 하기
//send를 연속으로 보낼 수 있는 구조가 되어야 한다.
class IOCP
{
	SOCKET						m_listenSocket;
	std::vector<ClientInfo>		m_clientInfos;
	std::vector<std::thread>	m_workerThreads;
	std::thread					m_accepterThread;
	bool						m_isWorkerRun;
	bool						m_isAccepterRun;
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

	//사용하지 않는 클라이언트 정보 구조체를 반환한다.
	ClientInfo* getEmptyClientInfo();


	void wokerThread();

	void accepterThread();

	void closeClient(ClientInfo* pClientInfo, bool bIsForce = false);

};