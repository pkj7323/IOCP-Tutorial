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
		std::cerr << "[����] WSAStartup : " << GetLastError() << std::endl;
		return false;
	}

	m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (m_listenSocket == INVALID_SOCKET)
	{
		std::cerr << "[����] WSASocket : " << GetLastError() << std::endl;
		return false;
	}

	std::cout << "���� ���� ����\n";
	return true;
}

bool IOCP::BindandListen(int nBindPort)
{
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(nBindPort);
		//���� ��Ʈ�� �����Ѵ�.        
		//� �ּҿ��� ������ �����̶� �޾Ƶ��̰ڴ�.
		//���� ������� �̷��� �����Ѵ�. ���� �� �����ǿ����� ������ �ް� �ʹٸ�
		//�� �ּҸ� inet_addr�Լ��� �̿��� ������ �ȴ�.
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int nRet = bind(m_listenSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(SOCKADDR_IN));
	if (nRet != 0)
	{
		std::cerr << "[����] bind()�Լ� ���� : " << GetLastError() << std::endl;
		return false;
	}

	nRet = listen(m_listenSocket, SOMAXCONN);
	if (nRet != 0)
	{
		std::cerr << "[����] listen()�Լ� ���� : " << GetLastError() << std::endl;
		return false;
	}

	std::cout << "���� ��� ����..\n";
	return true;
}

bool IOCP::StartServer(const UINT32 maxClientCount)
{
	createClient(maxClientCount);

	m_IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

	if (m_IOCPHandle == nullptr)
	{
		std::cerr << "[����] CreateIoCompletionPort()�Լ� ���� : " << GetLastError() << std::endl;
		return false;
	}

	bool bRet = createWorkerThread();
	if (bRet == false)
	{
		std::cerr << "[����] createWorkerThread()�Լ� ����" << std::endl;
		return false;
	}
	bRet = createAccepterThread();
	if (bRet == false)
	{
		std::cerr << "[����] createAccepterThread()�Լ� ����" << std::endl;
		return false;
	}
	std::cout << "��������" << std::endl;
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

	std::cout << "WokerThread ����..\n";
	return true;
}

bool IOCP::createAccepterThread()
{
	m_accepterThread = std::thread([this]() { accepterThread(); });

	std::cout << "AccepterThread ����..\n";
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
		std::cerr << "[����] CreateIoCompletionPort()�Լ� ���� : " << GetLastError() << std::endl;
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
		std::cerr << "[����] WSARecv()�Լ� ���� : " << WSAGetLastError() << std::endl;
		return false;
	}
	return true;
}

bool IOCP::sendMsg(ClientInfo* pClientInfo, char* pMsg, int nLen)
{
	DWORD dwRecvNumBytes = 0;

	std::memcpy(pClientInfo->m_SendBuf, pMsg, nLen);
	//nLen��ŭ pMsg�� m_szBuf�� ����

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
		std::cerr << "[����] WSASend()�Լ� ���� : " << WSAGetLastError() << std::endl;
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
		//�� �Լ��� ���� ��������� WaitingThread Queue��
		//��� ���·� ���� �ȴ�.
		//�Ϸ�� Overlapped I/O�۾��� �߻��ϸ� IOCP Queue����
		//�Ϸ�� �۾��� ������ �� ó���� �Ѵ�.
		//�׸��� PostQueuedCompletionStatus()�Լ��� ���� �����
		//�޼����� �����Ǹ� �����带 �����Ѵ�.
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
			std::cout << "socket(" << static_cast<int>(pClientInfo->m_socketClient) << ") ���� ����\n";
			closeClient(pClientInfo);
			continue;
		}

		OverlappedEx* pOverlappedEx = (OverlappedEx*)lpOverlapped;

		if (pOverlappedEx->m_eOperation == IOOperation::recv)
		{
			pClientInfo->m_RecvBuf[dwIoSize] = '\0';
			std::cout << "[����] bytes : " << dwIoSize << " , msg : " << pClientInfo->m_RecvBuf << std::endl;
			sendMsg(pClientInfo, pClientInfo->m_RecvBuf, dwIoSize);
			bindRecv(pClientInfo);
		}
		else if (pOverlappedEx->m_eOperation == IOOperation::send)
		{
			std::cout << "[�۽�] bytes : " << dwIoSize << " , msg : " << pClientInfo->m_SendBuf << std::endl;
		}
		else
		{
			std::cout << "socket(" << static_cast<int>(pClientInfo->m_socketClient) << ")���� ���ܻ�Ȳ\n";
		}
	}
}

void IOCP::accepterThread()
{
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(SOCKADDR_IN);

	while(m_isAccepterRun)
	{
		//���� ���� ����ü�� �ε��� �б�
		ClientInfo* pClientInfo = getEmptyClientInfo();

		if (!pClientInfo)
		{
			std::cerr << "Ŭ���̾�Ʈ FULL!" << std::endl;
			return;
		}

		//Ŭ���̾�Ʈ ���� ��û�� ���ö����� ��ٸ���.
		pClientInfo->m_socketClient = accept(m_listenSocket, reinterpret_cast<SOCKADDR*>(&clientAddr), &addrLen);

		if (pClientInfo->m_socketClient == INVALID_SOCKET)
		{
			continue;
		}

		bool ret = bindIOCompletionPort(pClientInfo);
		if (!ret)
		{
			std::cerr << "bindIOCompletionPort()�Լ� ����" << std::endl;
			return;
		}

		ret = bindRecv(pClientInfo);
		if (!ret)
		{
			std::cerr << "bindRecv()�Լ� ����" << std::endl;
			return;
		}
		std::array<char, 32> strIP={0};
		inet_ntop(AF_INET, (&clientAddr.sin_addr), strIP.data(), strIP.size() - 1);
		std::cout << "Ŭ���̾�Ʈ ���� : " << strIP.data() << "SOCKET: " << static_cast<int>(pClientInfo->m_socketClient) << std::endl;
		++m_clientCnt;
	}
}

void IOCP::closeClient(ClientInfo* pClientInfo, bool bIsForce)
{
	linger linger = { 0,0 };
	// SO_DONTLINGER�� ����

	// bIsForce�� true�̸� SO_LINGER, timeout = 0���� �����Ͽ� ���� ���� ��Ų��. ���� : ������ �ս��� ������ ����
	if (bIsForce)
	{
		linger.l_onoff = 1;
		linger.l_linger = 0;
	}

	// socketClose������ ������ �ۼ����� ��� �ߴ� ��Ų��.
	shutdown(pClientInfo->m_socketClient, SD_BOTH);

	// ���� �ɼ��� �����Ѵ�.
	setsockopt(pClientInfo->m_socketClient, SOL_SOCKET, SO_LINGER,
		reinterpret_cast<char*>(&linger), sizeof(linger));

	// ������ �ݴ´�.
	closesocket(pClientInfo->m_socketClient);

	pClientInfo->m_socketClient = INVALID_SOCKET;
}
