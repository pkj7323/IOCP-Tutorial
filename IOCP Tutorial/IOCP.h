#pragma once
#include "Define.h"


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
	~IOCP();
	bool InitSocket();
	//------서버용 함수-------
	//서버의 주소정보를 소켓과 연결시키고 접속 요청을 받기 위해 
	//소켓을 등록하는 함수
	bool BindandListen(int nBindPort);
	bool StartServer(const std::uint32_t maxClientCount);
	void DestroyThread();
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