#pragma once
#include "pch.h" 

static const std::uint32_t MAX_SOCKBUF = 256;
static const std::uint32_t MAX_WORKERTHREAD = 100;

enum class IOOperation
{
	recv,
	send
};

//overlapped I/O를 위해 구조체를 확장시킨다.
struct OverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;
	SOCKET        m_socketClient;
	WSABUF        m_wsaBuf;
	IOOperation   m_eOperation;
};

//TODO: 2 단계. OverlappedEx 구조체에 있는 char m_szBuf[ MAX_SOCKBUF ]를 stClientInfo 구조체로 이동 및 코드 분리하기
//앞 단계에는 OverlappedEx 구조체에 m_szBuf가 있어서 불 필요한 메모리 낭비가 발생함


struct ClientInfo
{
	SOCKET			m_socketClient;//클라이언트와 연결되는 소켓
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
	//------서버용 함수-------//
	//서버의 주소정보를 소켓과 연결시키고 접속 요청을 받기 위해 
	//소켓을 등록하는 함수
	bool BindandListen(int nBindPort);
	bool StartServer(const uint32_t maxClientCount);
	void DestroyThread();
private:
	void createClient(const uint32_t maxClientCount);
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