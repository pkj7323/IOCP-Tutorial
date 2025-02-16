#include "pch.h"
#include "IOCP.h"

IOCP::IOCP() : m_listenSocket{ INVALID_SOCKET }, m_isWorkerRun{ false },
m_isAccepterRun{ false }, m_isSenderRun{ false }, m_IOCPHandle{ INVALID_HANDLE_VALUE },
m_clientCnt{ 0 }
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
		std::cerr << "[����] WSAStartup : " << WSAGetLastError() << std::endl;
		return false;
	}

	m_listenSocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, 
		nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (m_listenSocket == INVALID_SOCKET)
	{
		std::cerr << "[����] WSASocket : " << WSAGetLastError() << std::endl;
		return false;
	}

	std::cout << "���� ���� ����\n";
	return true;
}

bool IOCP::BindandListen(int nBindPort)
{
	SOCKADDR_IN stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(nBindPort);
		//���� ��Ʈ�� �����Ѵ�.        
		//� �ּҿ��� ������ �����̶� �޾Ƶ��̰ڴ�.
		//���� ������� �̷��� �����Ѵ�. ���� �� �����ǿ����� ������ �ް� �ʹٸ�
		//�� �ּҸ� inet_addr�Լ��� �̿��� ������ �ȴ�.
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int nRet = bind(m_listenSocket, reinterpret_cast<SOCKADDR*>(&stServerAddr), sizeof(SOCKADDR_IN));
	if (nRet != 0)
	{
		std::cerr << "[����] bind()�Լ� ���� : " << WSAGetLastError() << std::endl;
		return false;
	}

	nRet = listen(m_listenSocket, 5);
	if (nRet != 0)
	{
		std::cerr << "[����] listen()�Լ� ���� : " << WSAGetLastError() << std::endl;
		return false;
	}

	std::cout << "���� ��� ����..\n";
	return true;
}

bool IOCP::StartServer(const UINT32& maxClientCount)
{
	createClient(maxClientCount);

	m_IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 
		nullptr, NULL, MAX_WORKERTHREAD);

	if (m_IOCPHandle == nullptr)
	{
		std::cerr << "[����] CreateIoCompletionPort()�Լ� ���� : " << GetLastError() << std::endl;
		return false;
	}

	bool bRet = createWorkerThread();
	if (bRet == false)
	{
		std::cerr << "[����] createWorkerThread()�Լ� ����\n" << std::endl;
		return false;
	}

	bRet = createAccepterThread();
	if (bRet == false)
	{
		std::cerr << "[����] createAccepterThread()�Լ� ����\n" << std::endl;
		return false;
	}

	createSendThread();

	std::cout << "��������" << std::endl;
	return true;

}

void IOCP::DestroyThread()
{
	m_isSenderRun = false;

	if (m_senderThread.joinable())
	{
		m_senderThread.join();
	}
	
	m_isAccepterRun = false;
	closesocket(m_listenSocket);

	if (m_accepterThread.joinable())
	{
		m_accepterThread.join();
	}


	m_isWorkerRun = false;
	CloseHandle(m_IOCPHandle);

	for (auto& th : m_workerThreads)
	{
		if (th.joinable())
		{
			th.join();
		}
	}

}


bool IOCP::SendMsg(const UINT32& sessionIndex_, const UINT32& dataSize_, char* pData)
{
	auto pClient = GetClientInfo(sessionIndex_);
	return pClient->SendMsg(dataSize_, pData);
}

void IOCP::createClient(const UINT32& maxClientCount)
{
	for (UINT32 i = 0; i < maxClientCount; ++i)
	{
		auto client = new ClientInfo();
		client->Init(i);

		m_clientInfos.push_back(client);
	}
}


bool IOCP::createWorkerThread()
{
	m_isWorkerRun = true;

	for (UINT32 i = 0; i < MAX_WORKERTHREAD; i++)
	{
		m_workerThreads.emplace_back([this]() { wokerThread(); });
	}

	std::cout << "WokerThread ����..\n";
	return true;
}

bool IOCP::createAccepterThread()
{
	m_isAccepterRun = true;
	m_accepterThread = std::thread([this]() { accepterThread(); });

	std::cout << "AccepterThread ����..\n";
	return true;
}

void IOCP::createSendThread()
{
	m_isSenderRun = true;
	m_senderThread = std::thread([this]() { SendThread(); });
	std::cout << "SenderThread ����..\n";
}


ClientInfo* IOCP::getEmptyClientInfo()
{
	for(auto client : m_clientInfos)
	{
		if (client->IsConnected() == false)
		{
			return client;
		}
	}

	return nullptr;
}

ClientInfo* IOCP::GetClientInfo(const UINT32& sessionIndex)
{
	return m_clientInfos[sessionIndex];
}

void IOCP::closeSocket(ClientInfo* pClientInfo, bool bIsForce)
{
	auto clientIndex = pClientInfo->GetIndex();

	pClientInfo->Close(bIsForce);

	OnClose(clientIndex);
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
			reinterpret_cast<PULONG_PTR>(&pClientInfo), 
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
			closeSocket(pClientInfo);
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

		if (pClientInfo == nullptr)
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

		if (pClientInfo->OnConnect(m_IOCPHandle, newSocket) == false)
		{
			pClientInfo->Close(true);
			return; //contiune���� �������� �ٲ���� �Ǽ����� �� 
		}
		/*
		std::array<char, 32> strIP={0};
		inet_ntop(AF_INET, (&clientAddr.sin_addr), strIP.data(), strIP.size() - 1);
		std::cout << "Ŭ���̾�Ʈ ���� : " << strIP.data() << "SOCKET: " << static_cast<int>(pClientInfo->m_socketClient) << std::endl;*/

		OnConnect(pClientInfo->GetIndex());

		++m_clientCnt;
	}
}

void IOCP::SendThread()
{
	while (m_isSenderRun)
	{
		for (auto client : m_clientInfos)
		{
			if (client->IsConnected() == false)
			{
				continue;
			}
			client->SendIO();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

