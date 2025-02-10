#include "IOCP.h"

IOCP::IOCP() : m_listenSocket{ INVALID_SOCKET }, m_isWorkerRun{ true },
m_isAccepterRun{ true }, m_IOCPHandle{ INVALID_HANDLE_VALUE }, m_clientCnt{ 0 }
{
}


IOCP::~IOCP()
{
	WSACleanup();
}

bool IOCP::InitSocket()
{
	WSADATA wsaData;

	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRet != 0)
	{
		std::cerr << "[에러] WSAStartup : " << GetLastError() << std::endl;
		return false;
	}

	m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (m_listenSocket == INVALID_SOCKET)
	{
		std::cerr << "[에러] WSASocket : " << GetLastError() << std::endl;
		return false;
	}

	std::cout << "소켓 연결 성공\n";
	return true;
}

bool IOCP::BindandListen(int nBindPort)
{
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(nBindPort);
		//서버 포트를 설정한다.        
		//어떤 주소에서 들어오는 접속이라도 받아들이겠다.
		//보통 서버라면 이렇게 설정한다. 만약 한 아이피에서만 접속을 받고 싶다면
		//그 주소를 inet_addr함수를 이용해 넣으면 된다.
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int nRet = bind(m_listenSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(SOCKADDR_IN));
	if (nRet != 0)
	{
		std::cerr << "[에러] bind()함수 실패 : " << GetLastError() << std::endl;
		return false;
	}

	nRet = listen(m_listenSocket, SOMAXCONN);
	if (nRet != 0)
	{
		std::cerr << "[에러] listen()함수 실패 : " << GetLastError() << std::endl;
		return false;
	}

	std::cout << "서버 등록 성공..\n";
	return true;
}

bool IOCP::StartServer(const UINT32 maxClientCount)
{
	createClient(maxClientCount);

	m_IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

	if (m_IOCPHandle == nullptr)
	{
		std::cerr << "[에러] CreateIoCompletionPort()함수 실패 : " << GetLastError() << std::endl;
		return false;
	}

	bool bRet = createWorkerThread();
	if (bRet == false)
	{
		std::cerr << "[에러] createWorkerThread()함수 실패" << std::endl;
		return false;
	}
	bRet = createAccepterThread();
	if (bRet == false)
	{
		std::cerr << "[에러] createAccepterThread()함수 실패" << std::endl;
		return false;
	}
	std::cout << "서버시작" << std::endl;
	return true;

}

void IOCP::DestroyThread()
{
	m_isWorkerRun = false;
	CloseHandle(m_IOCPHandle);

	for (auto& th : m_workerThreads)
	{
		if (th.joinable())
		{
			th.join();
		}
	}
	m_isAccepterRun = false;
	closesocket(m_listenSocket);

	if (m_accepterThread.joinable())
	{
		m_accepterThread.join();
	}

}

void IOCP::createClient(const UINT32 maxClientCount)
{
	for (UINT32 i = 0; i < maxClientCount; ++i)
	{
		m_clientInfos.emplace_back();
	}
}


bool IOCP::createWorkerThread()
{
	for (int i = 0; i < MAX_WORKERTHREAD; i++)
	{
		m_workerThreads.emplace_back([this]() { wokerThread(); });
	}

	std::cout << "WokerThread 시작..\n";
	return true;
}

bool IOCP::createAccepterThread()
{
	m_accepterThread = std::thread([this]() { accepterThread(); });

	std::cout << "AccepterThread 시작..\n";
	return true;
}


ClientInfo* IOCP::getEmptyClientInfo()
{
	for(auto& client : m_clientInfos)
	{
		if (client.m_socketClient == INVALID_SOCKET)
		{
			return &client;
		}
	}
	return nullptr;
}


bool IOCP::bindIOCompletionPort(ClientInfo* pClientInfo)
{
	auto hIOCP = CreateIoCompletionPort(
		reinterpret_cast<HANDLE>(pClientInfo->m_socketClient),
		m_IOCPHandle,
		reinterpret_cast<ULONG_PTR>(pClientInfo),
		0
	);
	if (hIOCP == nullptr && m_IOCPHandle != hIOCP)
	{
		std::cerr << "[에러] CreateIoCompletionPort()함수 실패 : " << GetLastError() << std::endl;
		return false;
	}
	return true;
}

bool IOCP::bindRecv(ClientInfo* pClientInfo)
{
	DWORD dwFlags = 0;
	DWORD dwRecvNumBytes = 0;

	pClientInfo->m_RecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
	pClientInfo->m_RecvOverlappedEx.m_wsaBuf.buf = pClientInfo->m_RecvBuf;
	pClientInfo->m_RecvOverlappedEx.m_eOperation = IOOperation::recv;

	int nRet = WSARecv(
		pClientInfo->m_socketClient,
		&(pClientInfo->m_RecvOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		&dwFlags,
		reinterpret_cast<LPWSAOVERLAPPED>(&pClientInfo->m_RecvOverlappedEx),
		nullptr);

	if (nRet == SOCKET_ERROR && (GetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[에러] WSARecv()함수 실패 : " << WSAGetLastError() << std::endl;
		return false;
	}
	return true;
}

bool IOCP::sendMsg(ClientInfo* pClientInfo, char* pMsg, int nLen)
{
	DWORD dwRecvNumBytes = 0;

	std::memcpy(pClientInfo->m_SendBuf, pMsg, nLen);
	//nLen만큼 pMsg를 m_szBuf로 복사

	pClientInfo->m_SendOverlappedEx.m_wsaBuf.len = nLen;
	pClientInfo->m_SendOverlappedEx.m_wsaBuf.buf = pClientInfo->m_SendBuf;
	pClientInfo->m_SendOverlappedEx.m_eOperation = IOOperation::send;

	int nRet = WSASend(
		pClientInfo->m_socketClient,
		&(pClientInfo->m_SendOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		reinterpret_cast<LPWSAOVERLAPPED>(&pClientInfo->m_SendOverlappedEx),
		nullptr
	);

	if (nRet == SOCKET_ERROR && (GetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[에러] WSASend()함수 실패 : " << WSAGetLastError() << std::endl;
		return false;
	}
	return true;
}

void IOCP::wokerThread()
{
	ClientInfo* pClientInfo = nullptr;

	bool bSuccess = true;

	DWORD dwIoSize = 0;

	LPOVERLAPPED lpOverlapped = nullptr;

	while (m_isWorkerRun)
	{
		//////////////////////////////////////////////////////
		//이 함수로 인해 쓰레드들은 WaitingThread Queue에
		//대기 상태로 들어가게 된다.
		//완료된 Overlapped I/O작업이 발생하면 IOCP Queue에서
		//완료된 작업을 가져온 뒤 처리를 한다.
		//그리고 PostQueuedCompletionStatus()함수에 의해 사용자
		//메세지가 도착되면 쓰레드를 종료한다.
		//////////////////////////////////////////////////////

		bSuccess = GetQueuedCompletionStatus(
			m_IOCPHandle, 
			&dwIoSize, 
			(PULONG_PTR)&pClientInfo, 
			&lpOverlapped, 
			INFINITE
		);

		if (bSuccess && dwIoSize == 0 && !lpOverlapped)
		{
			m_isWorkerRun = false;
			continue;
		}

		if (!lpOverlapped)
		{
			continue;
		}

		if (!bSuccess || (bSuccess && dwIoSize == 0))
		{
			std::cout << "socket(" << static_cast<int>(pClientInfo->m_socketClient) << ") 접속 끊김\n";
			closeClient(pClientInfo);
			continue;
		}

		OverlappedEx* pOverlappedEx = (OverlappedEx*)lpOverlapped;

		if (pOverlappedEx->m_eOperation == IOOperation::recv)
		{
			pClientInfo->m_RecvBuf[dwIoSize] = '\0';
			std::cout << "[수신] bytes : " << dwIoSize << " , msg : " << pClientInfo->m_RecvBuf << std::endl;
			sendMsg(pClientInfo, pClientInfo->m_RecvBuf, dwIoSize);
			bindRecv(pClientInfo);
		}
		else if (pOverlappedEx->m_eOperation == IOOperation::send)
		{
			std::cout << "[송신] bytes : " << dwIoSize << " , msg : " << pClientInfo->m_SendBuf << std::endl;
		}
		else
		{
			std::cout << "socket(" << static_cast<int>(pClientInfo->m_socketClient) << ")에서 예외상황\n";
		}
	}
}

void IOCP::accepterThread()
{
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(SOCKADDR_IN);

	while(m_isAccepterRun)
	{
		//접속 받을 구조체의 인덱스 읽기
		ClientInfo* pClientInfo = getEmptyClientInfo();

		if (!pClientInfo)
		{
			std::cerr << "클라이언트 FULL!" << std::endl;
			return;
		}

		//클라이언트 접속 요청이 들어올때까지 기다린다.
		pClientInfo->m_socketClient = accept(m_listenSocket, reinterpret_cast<SOCKADDR*>(&clientAddr), &addrLen);

		if (pClientInfo->m_socketClient == INVALID_SOCKET)
		{
			continue;
		}

		bool ret = bindIOCompletionPort(pClientInfo);
		if (!ret)
		{
			std::cerr << "bindIOCompletionPort()함수 실패" << std::endl;
			return;
		}

		ret = bindRecv(pClientInfo);
		if (!ret)
		{
			std::cerr << "bindRecv()함수 실패" << std::endl;
			return;
		}
		std::array<char, 32> strIP={0};
		inet_ntop(AF_INET, (&clientAddr.sin_addr), strIP.data(), strIP.size() - 1);
		std::cout << "클라이언트 접속 : " << strIP.data() << "SOCKET: " << static_cast<int>(pClientInfo->m_socketClient) << std::endl;
		++m_clientCnt;
	}
}

void IOCP::closeClient(ClientInfo* pClientInfo, bool bIsForce)
{
	linger linger = { 0,0 };
	// SO_DONTLINGER로 설정

	// bIsForce가 true이면 SO_LINGER, timeout = 0으로 설정하여 강제 종료 시킨다. 주의 : 데이터 손실이 있을수 있음
	if (bIsForce)
	{
		linger.l_onoff = 1;
		linger.l_linger = 0;
	}

	// socketClose소켓의 데이터 송수신을 모두 중단 시킨다.
	shutdown(pClientInfo->m_socketClient, SD_BOTH);

	// 소켓 옵션을 설정한다.
	setsockopt(pClientInfo->m_socketClient, SOL_SOCKET, SO_LINGER,
		reinterpret_cast<char*>(&linger), sizeof(linger));

	// 소켓을 닫는다.
	closesocket(pClientInfo->m_socketClient);

	pClientInfo->m_socketClient = INVALID_SOCKET;
}
