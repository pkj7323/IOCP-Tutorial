#pragma once
#include "pch.h" 

#define MAX_SOCKBUF 1024
#define MAX_WORKERTHREAD 4

enum class IOOperation
{
	recv,
	send
};

//overlapped I/O�� ���� ����ü�� Ȯ���Ų��.
struct OverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;
	SOCKET        m_socketClient;
	WSABUF        m_wsaBuf;
	char          m_szBuf[MAX_SOCKBUF];
	IOOperation   m_eOperation;
};

struct ClientInfo
{
	SOCKET m_socketClient;//Ŭ���̾�Ʈ�� ����Ǵ� ����
	OverlappedEx m_RecvOverlappedEx;
	OverlappedEx m_SendOverlappedEx;

	ClientInfo()
	{
		memset(&m_RecvOverlappedEx, 0, sizeof(OverlappedEx));
		memset(&m_SendOverlappedEx, 0, sizeof(OverlappedEx));
		m_socketClient = INVALID_SOCKET;
	}
};

class IOCP
{
	SOCKET m_listenSocket;
	std::vector<ClientInfo> m_clientInfos;
	std::vector<std::thread> m_workerThreads;
	std::thread m_accepterThread;
	bool m_isWorkerRun;
	bool m_isAccepterRun;
	HANDLE m_IOCPHandle;
	int m_clientCnt;
public:
	IOCP();
	~IOCP();
	bool InitSocket();
	//------������ �Լ�-------//
	//������ �ּ������� ���ϰ� �����Ű�� ���� ��û�� �ޱ� ���� 
	//������ ����ϴ� �Լ�
	bool BindandListen(int nBindPort);
	bool StartServer(const uint32_t maxClientCount);
	void DestroyThread();
private:
	void createClient(const uint32_t maxClientCount);
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