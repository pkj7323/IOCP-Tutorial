#pragma once
#include "pch.h"
#include "Define.h"
#include "ClientInfo.h"

//TODO: 5 �ܰ�. 1-Send �����ϱ�
// ���ۿ� �׾� ����, send �����忡�� ������.
//TODO: ���� �߻� Ŭ���̾�Ʈ���� �ι����� �޼��� ������ �پȺ������� ���� �߻� �׽�Ʈ �ڵ�� �ߵ��ư�
/// ����§ �ڵ常 ����

class IOCP
{
	std::vector<ClientInfo*>	m_clientInfos;
	//Ŭ���̾�Ʈ ���� ���� ����ü

	SOCKET						m_listenSocket;
	//Ŭ���̾�Ʈ�� ������ �ޱ����� ���� ����

	int							m_clientCnt;
	//���ӵ� Ŭ���̾�Ʈ ��

	bool						m_isWorkerRun;//��Ŀ �����尡 �����÷���
	std::vector<std::thread>	m_workerThreads;//��Ŀ ������

	bool						m_isAccepterRun;//����� �����尡 �����÷���
	std::thread					m_accepterThread;//����� ������

	bool						m_isSenderRun;//������ ������ �����÷���
	std::thread					m_senderThread;//������ ������

	HANDLE						m_IOCPHandle;//IOCP �ڵ�

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