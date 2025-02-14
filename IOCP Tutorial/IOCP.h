#pragma once
#include "Define.h"

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

	//������� �ʴ� Ŭ���̾�Ʈ ���� ����ü�� ��ȯ�Ѵ�.
	ClientInfo* getEmptyClientInfo();

	//CompletionPort��ü�� ���ϰ� CompletionKey�� �����Ű�� ������ �Ѵ�.
	bool bindIOCompletionPort(ClientInfo* pClientInfo);

	bool bindRecv(ClientInfo* pClientInfo);

	bool sendMsg(ClientInfo* pClientInfo, char* pMsg, int nLen);

	void wokerThread();

	void accepterThread();

	void closeClient(ClientInfo* pClientInfo, bool bIsForce = false);

};