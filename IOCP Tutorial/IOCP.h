#pragma once
#include "pch.h"
#include "Define.h"
#include "ClientInfo.h"

/*TODO:
7 �ܰ�.�񵿱� Accept ����ϱ�
6�ܰ������ Accept ó���� ���� I / O�� �ߴ�.�̰��� �񵿱�I / O�� �ٲ۴�.�̷ν� ��Ʈ��ũ ������ ��� �񵿱� I / O�� �ȴ�
6�ܰ迡�� �̾ ����� �����Ѵ�.*/


class IOCP
{
	UINT32						MaxIOWorkerThreadCnt = 0;

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

	HANDLE						m_IOCPHandle;//IOCP �ڵ�

public:
	IOCP();
	virtual ~IOCP();
	bool Init(const UINT32& maxIOWorkerThreadCnt_);

	//------������ �Լ�-------
	//������ �ּ������� ���ϰ� �����Ű�� ���� ��û�� �ޱ� ���� 
	//������ ����ϴ� �Լ�
	bool BindandListen(int nBindPort);

	bool StartServer(const UINT32& maxClientCount);

	void DestroyThread();


	bool SendMsg(const UINT32& sessionIndex_, const UINT32& dataSize_, char* pData);

	virtual void OnConnect(const UINT32& clientIndex) = 0;
	virtual void OnClose(const UINT32& clientIndex) = 0;
	virtual void OnReceive(const UINT32& clientIndex, const UINT32& size, char* pData) = 0;
private:
	void createClient(const UINT32& maxClientCount);

	bool createWorkerThread();

	bool createAccepterThread();

	ClientInfo* GetClientInfo(const UINT32& sessionIndex)
	{
		return m_clientInfos[sessionIndex];
	}


	//������� �ʴ� Ŭ���̾�Ʈ ���� ����ü�� ��ȯ�Ѵ�.
	ClientInfo* getEmptyClientInfo();

	void closeSocket(ClientInfo* pClientInfo, bool bIsForce = false);

	void wokerThread();

	void accepterThread();


};