#pragma once
#include "Define.h"
#include "ClientInfo.h"

//TODO: 4 �ܰ�. ��Ʈ��ũ�� ����(��Ŷ or ��û) ó�� ������ ������� �и��ϱ�
//Send�� Recv�� �ٸ� �����忡�� �ϱ�
//send�� �������� ���� �� �ִ� ������ �Ǿ�� �Ѵ�.
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

	//������� �ʴ� Ŭ���̾�Ʈ ���� ����ü�� ��ȯ�Ѵ�.
	ClientInfo* getEmptyClientInfo();


	void wokerThread();

	void accepterThread();

	void closeClient(ClientInfo* pClientInfo, bool bIsForce = false);

};