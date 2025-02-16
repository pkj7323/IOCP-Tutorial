#pragma once
#include "pch.h"
#include "Define.h"
#include "ClientInfo.h"

//TODO: 5 단계. 1-Send 구현하기
// 버퍼에 쌓아 놓고, send 스레드에서 보내기.
//TODO: 문제 발생 클라이언트에서 두번정도 메세지 보내면 다안보내지는 버그 발생 테스트 코드는 잘돌아감
/// 내가짠 코드만 문제

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

	bool						m_isSenderRun;//보내는 스레드 동작플래그
	std::thread					m_senderThread;//보내는 스레드

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