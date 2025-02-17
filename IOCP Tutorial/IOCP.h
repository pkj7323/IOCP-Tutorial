#pragma once
#include "pch.h"
#include "Define.h"
#include "ClientInfo.h"

//6 단계. 1 - Send 구현하기
//queue에 담아 놓고 순차적으로 보내기.

class IOCP
{
	std::vector<ClientInfo*>	m_clientInfos;
	//클라이언트 정보 저장 구조체

	SOCKET						m_listenSocket;
	//클라이언트의 접속을 받기위한 리슨 소켓

	int							m_clientCnt;
	//접속된 클라이언트 수

	bool						m_isWorkerRun;//워커 스레드가 동작플래그
	std::vector<std::thread>	m_workerThreads;//워커 스레드

	bool						m_isAccepterRun;//어셉터 스레드가 동작플래그
	std::thread					m_accepterThread;//어셉터 스레드

	HANDLE						m_IOCPHandle;//IOCP 핸들

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


	bool SendMsg(const UINT32& sessionIndex_, const UINT32& dataSize_, char* pData);

	virtual void OnConnect(const UINT32& clientIndex) {}
	virtual void OnClose(const UINT32& clientIndex) {}
	virtual void OnReceive(const UINT32& clientIndex, const UINT32& size, char* pData) {}
private:
	void createClient(const UINT32& maxClientCount);

	bool createWorkerThread();

	bool createAccepterThread();

	ClientInfo* GetClientInfo(const UINT32& sessionIndex)
	{
		return m_clientInfos[sessionIndex];
	}


	//사용하지 않는 클라이언트 정보 구조체를 반환한다.
	ClientInfo* getEmptyClientInfo();

	void closeSocket(ClientInfo* pClientInfo, bool bIsForce = false);

	void wokerThread();

	void accepterThread();


};