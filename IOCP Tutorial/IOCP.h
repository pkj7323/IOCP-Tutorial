#pragma once
#include "Define.h"

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

	bool StartServer(const std::uint32_t maxClientCount);

	void DestroyThread();

	ClientInfo* GetClientInfo(const uint32_t& sessionIndex);

	bool SendMsg(const std::uint32_t& sessionIndex_, const std::uint32_t& dataSize_ , char* pData);

	virtual void OnConnect(const std::uint32_t clientIndex) {}
	virtual void OnClose(const std::uint32_t clientIndex) {}
	virtual void OnReceive(const std::uint32_t clientIndex, const std::uint32_t size, char* pData) {}
private:
	void createClient(const std::uint32_t maxClientCount);

	bool createWorkerThread();

	bool createAccepterThread();

	//사용하지 않는 클라이언트 정보 구조체를 반환한다.
	ClientInfo* getEmptyClientInfo();

	//CompletionPort객체와 소켓과 CompletionKey를 연결시키는 역할을 한다.
	bool bindIOCompletionPort(ClientInfo* pClientInfo);

	bool bindRecv(ClientInfo* pClientInfo);

	bool sendMsg(ClientInfo* pClientInfo, char* pMsg, int nLen);

	void wokerThread();

	void accepterThread();

	void closeClient(ClientInfo* pClientInfo, bool bIsForce = false);

};