#include "pch.h"
#include "IOCP.h"

IOCP::IOCP() : m_listenSocket{ INVALID_SOCKET }, m_clientCnt{0},
               m_isWorkerRun{true}, m_isAccepterRun{true},
               m_IOCPHandle{INVALID_HANDLE_VALUE}
{
}

IOCP::~IOCP()
{
	WSACleanup();
}

bool IOCP::Init(const UINT32& maxIOWorkerThreadCnt_)
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

	MaxIOWorkerThreadCnt = maxIOWorkerThreadCnt_;

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

	m_IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr,
		NULL, MaxIOWorkerThreadCnt);
	if (m_IOCPHandle == nullptr)
	{
		std::cerr << "[����] CreateIoCompletionPort()�Լ� ���� : " << GetLastError() << std::endl;
		return false;
	}
	auto hIOCP = CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_listenSocket), 
		m_IOCPHandle,0, 0);
	if (hIOCP == nullptr)
	{
		std::cerr << "[����] listen socket IOCP bind ����: " << WSAGetLastError() << std::endl;
		return false;
	}

	std::cout << "���� ��� ����..\n";
	return true;
}

bool IOCP::StartServer(const UINT32& maxClientCount)
{
	createClient(maxClientCount);

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


bool IOCP::SendMsg(const UINT32& sessionIndex_, const UINT32& dataSize_, char* pData)
{
	auto pClient = GetClientInfo(sessionIndex_);
	return pClient->SendMsg(dataSize_, pData);
}

void IOCP::createClient(const UINT32& maxClientCount)
{
	for (UINT32 i = 0; i < maxClientCount; ++i)
	{
		m_clientInfos.emplace_back(new ClientInfo(i, m_IOCPHandle));
	}
}


bool IOCP::createWorkerThread()
{
	for (UINT32 i = 0; i < MaxIOWorkerThreadCnt; i++)
	{
		m_workerThreads.emplace_back([this]() { wokerThread(); });
	}

	std::cout << "WokerThread ����..\n";
	return true;
}
ClientInfo* IOCP::getEmptyClientInfo()
{
	for(auto& client : m_clientInfos)
	{
		if (client->IsConnected() == false)
		{
			return client;
		}
	}

	return nullptr;
}

bool IOCP::createAccepterThread()
{
	m_accepterThread = std::thread([this]() { accepterThread(); });

	std::cout << "AccepterThread ����..\n";
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
			&dwIoSize, //������ ���۵� ����Ʈ
			reinterpret_cast<PULONG_PTR>(&pClientInfo), //����� ������ completion key
			&lpOverlapped, //�Ϸ�� I/O�۾��� ����
			INFINITE//����� �ð�
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
		auto pOverlappedEx = reinterpret_cast<OverlappedEx*>(lpOverlapped);


		if (!bSuccess || (0 == dwIoSize && IOOperation::ACCEPT != pOverlappedEx->m_eOperation))
		{
			//std::cout << "socket(" << static_cast<int>(pClientInfo->GetIndex()) << ")���� ���ܻ�Ȳ\n";
			closeSocket(pClientInfo);
			continue;
		}

		if (pOverlappedEx->m_eOperation == IOOperation::ACCEPT)
		{
			pClientInfo = GetClientInfo(pOverlappedEx->m_sessionIndex);
			if (pClientInfo->AcceptCompletion())
			{
				++m_clientCnt;

				OnConnect(pClientInfo->GetIndex());
			}
			else
			{
				closeSocket(pClientInfo, true);
			}
		}
		else if (pOverlappedEx->m_eOperation == IOOperation::RECV)
		{
			OnReceive(pClientInfo->GetIndex(), dwIoSize, pClientInfo->GetRecvBuffer());

			pClientInfo->BindRecv();
		}
		else if (pOverlappedEx->m_eOperation == IOOperation::SEND)
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
	while (m_isAccepterRun)
	{
		auto curTimeSec = std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();

		for (auto client : m_clientInfos)
		{
			if (client->IsConnected())
			{
				continue;
			}
			if (static_cast<UINT64>(curTimeSec) < client->GetLastestClosedTimeSec())
			{
				continue;
			}
			auto diff = curTimeSec - client->GetLastestClosedTimeSec();
			if (diff <= RE_USE_SESSION_WAIT_TIMESEC)
			{
				continue;
			}
			client->PostAccept(m_listenSocket, curTimeSec);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(32));
	}
}



void IOCP::closeSocket(ClientInfo* pClientInfo, bool bIsForce)
{
	if (pClientInfo->IsConnected() == false)
	{
		return;
	}
	auto clientIndex = pClientInfo->GetIndex();

	pClientInfo->Close(bIsForce);

	OnClose(clientIndex);
}


