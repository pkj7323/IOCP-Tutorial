#pragma once
#include "pch.h" 

static const std::uint32_t MAX_SOCKBUF = 256;
static const std::uint32_t MAX_WORKERTHREAD = 100;

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
	IOOperation   m_eOperation;
};

//TODO: 2 �ܰ�. OverlappedEx ����ü�� �ִ� char m_szBuf[ MAX_SOCKBUF ]�� stClientInfo ����ü�� �̵� �� �ڵ� �и��ϱ�
//�� �ܰ迡�� OverlappedEx ����ü�� m_szBuf�� �־ �� �ʿ��� �޸� ���� �߻���


struct ClientInfo
{
	SOCKET			m_socketClient;//Ŭ���̾�Ʈ�� ����Ǵ� ����
	OverlappedEx	m_RecvOverlappedEx;
	OverlappedEx	m_SendOverlappedEx;

	char			m_RecvBuf[MAX_SOCKBUF];
	char			m_SendBuf[MAX_SOCKBUF];

	ClientInfo()
	{
		ZeroMemory(&m_RecvOverlappedEx, sizeof(OverlappedEx));
		ZeroMemory(&m_SendOverlappedEx, sizeof(OverlappedEx));
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