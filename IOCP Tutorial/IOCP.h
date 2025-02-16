#pragma once
#include "pch.h"
#include "Define.h"
#include "ClientInfo.h"

//TODO: 5 �ܰ�. 1-Send �����ϱ�
// ���ۿ� �׾� ����, send �����忡�� ������.

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

	//------������ �Լ�-------
	//������ �ּ������� ���ϰ� �����Ű�� ���� ��û�� �ޱ� ���� 
	//������ ����ϴ� �Լ�
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

	//������� �ʴ� Ŭ���̾�Ʈ ���� ����ü�� ��ȯ�Ѵ�.
	ClientInfo* getEmptyClientInfo();

	void closeSocket(ClientInfo* pClientInfo, bool bIsForce = false);

	void wokerThread();

	void accepterThread();

	void SendThread();
};