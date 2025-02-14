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

ClientInfo* IOCP::GetClientInfo(const uint32_t& sessionIndex)
{
	return &m_clientInfos[sessionIndex];
}

bool IOCP::SendMsg(const std::uint32_t& sessionIndex_, const std::uint32_t& dataSize_, char* pData)
{
	auto pClient = GetClientInfo(sessionIndex_);
	return pClient->SendMsg(dataSize_, pData);
}

void IOCP::createClient(const UINT32 maxClientCount)
{
	for (UINT32 i = 0; i < maxClientCount; ++i)
	{
		m_clientInfos.emplace_back();
		m_clientInfos[i].Init(i);
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
		if (client.IsConnected() == false)
		{
			return &client;
		}
	}
	return nullptr;
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

		if (bSuccess && dwIoSize == 0 && lpOverlapped == nullptr)
		{
			m_isWorkerRun = false;
			continue;
		}

		if (lpOverlapped == nullptr)
		{
			continue;
		}

		if (!bSuccess || (bSuccess && dwIoSize == 0))
		{
			//std::cout << "socket(" << static_cast<int>(pClientInfo->GetSocket()) << ") ���� ����\n";
			closeClient(pClientInfo);
			continue;
		}

		auto pOverlappedEx = reinterpret_cast<OverlappedEx*>(lpOverlapped);

		if (pOverlappedEx->m_eOperation == IOOperation::recv)
		{
			OnReceive(pClientInfo->GetIndex(), dwIoSize, pClientInfo->GetRecvBuffer());

			pClientInfo->BindRecv();
		}
		else if (pOverlappedEx->m_eOperation == IOOperation::send)
		{
			delete[] pOverlappedEx->m_wsaBuf.buf;
			delete pOverlappedEx;
			pClientInfo->SendCompleted(dwIoSize);
		}
		else
		{
			std::cout << "socket(" << static_cast<int>(pClientInfo->GetIndex()) << ")���� ���ܻ�Ȳ\n";
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
		auto newSocket = accept(m_listenSocket, reinterpret_cast<SOCKADDR*>(&clientAddr), &addrLen);

		if (newSocket == INVALID_SOCKET)
		{
			continue;
		}

		if (pClientInfo->OnConnect(m_IOCPHandle,newSocket) == false)
		{
			pClientInfo->Close(true);
			continue;
		}
		/*
		std::array<char, 32> strIP={0};
		inet_ntop(AF_INET, (&clientAddr.sin_addr), strIP.data(), strIP.size() - 1);
		std::cout << "Ŭ���̾�Ʈ ���� : " << strIP.data() << "SOCKET: " << static_cast<int>(pClientInfo->m_socketClient) << std::endl;*/

		OnConnect(pClientInfo->GetIndex());

		++m_clientCnt;
	}
}

void IOCP::closeClient(ClientInfo* pClientInfo, bool bIsForce)
{
	auto clientIndex = pClientInfo->GetIndex();

	pClientInfo->Close(bIsForce);

	OnClose(clientIndex);
}
