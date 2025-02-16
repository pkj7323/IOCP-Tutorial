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
		std::cerr << "[에러] WSAStartup : " << WSAGetLastError() << std::endl;
		return false;
	}

	m_listenSocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, 
		nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (m_listenSocket == INVALID_SOCKET)
	{
		std::cerr << "[에러] WSASocket : " << WSAGetLastError() << std::endl;
		return false;
	}

	std::cout << "소켓 연결 성공\n";
	return true;
}

bool IOCP::BindandListen(int nBindPort)
{
	SOCKADDR_IN stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(nBindPort);
		//서버 포트를 설정한다.        
		//어떤 주소에서 들어오는 접속이라도 받아들이겠다.
		//보통 서버라면 이렇게 설정한다. 만약 한 아이피에서만 접속을 받고 싶다면
		//그 주소를 inet_addr함수를 이용해 넣으면 된다.
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int nRet = bind(m_listenSocket, reinterpret_cast<SOCKADDR*>(&stServerAddr), sizeof(SOCKADDR_IN));
	if (nRet != 0)
	{
		std::cerr << "[에러] bind()함수 실패 : " << WSAGetLastError() << std::endl;
		return false;
	}

	nRet = listen(m_listenSocket, 5);
	if (nRet != 0)
	{
		std::cerr << "[에러] listen()함수 실패 : " << WSAGetLastError() << std::endl;
		return false;
	}

	std::cout << "서버 등록 성공..\n";
	return true;
}

bool IOCP::StartServer(const UINT32& maxClientCount)
{
	createClient(maxClientCount);

	m_IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 
		nullptr, NULL, MAX_WORKERTHREAD);

	if (m_IOCPHandle == nullptr)
	{
		std::cerr << "[에러] CreateIoCompletionPort()함수 실패 : " << GetLastError() << std::endl;
		return false;
	}

	bool bRet = createWorkerThread();
	if (bRet == false)
	{
		std::cerr << "[에러] createWorkerThread()함수 실패\n" << std::endl;
		return false;
	}

	bRet = createAccepterThread();
	if (bRet == false)
	{
		std::cerr << "[에러] createAccepterThread()함수 실패\n" << std::endl;
		return false;
	}

	createSendThread();

	std::cout << "서버시작" << std::endl;
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

	std::cout << "WokerThread 시작..\n";
	return true;
}

bool IOCP::createAccepterThread()
{
	m_isAccepterRun = true;
	m_accepterThread = std::thread([this]() { accepterThread(); });

	std::cout << "AccepterThread 시작..\n";
	return true;
}

void IOCP::createSendThread()
{
	m_isSenderRun = true;
	m_senderThread = std::thread([this]() { SendThread(); });
	std::cout << "SenderThread 시작..\n";
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
			//std::cout << "socket(" << static_cast<int>(pClientInfo->GetSocket()) << ") 접속 끊김\n";
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
			std::cout << "socket(" << static_cast<int>(pClientInfo->GetIndex()) << ")에서 예외상황\n";
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

		if (pClientInfo == nullptr)
		{
			std::cerr << "클라이언트 FULL!" << std::endl;
			return;
		}

		//클라이언트 접속 요청이 들어올때까지 기다린다.
		auto newSocket = accept(m_listenSocket, reinterpret_cast<SOCKADDR*>(&clientAddr), &addrLen);

		if (newSocket == INVALID_SOCKET)
		{
			continue;
		}

		if (pClientInfo->OnConnect(m_IOCPHandle, newSocket) == false)
		{
			pClientInfo->Close(true);
			return; //contiune에서 리턴으로 바뀐건지 실수인지 모름 
		}
		/*
		std::array<char, 32> strIP={0};
		inet_ntop(AF_INET, (&clientAddr.sin_addr), strIP.data(), strIP.size() - 1);
		std::cout << "클라이언트 접속 : " << strIP.data() << "SOCKET: " << static_cast<int>(pClientInfo->m_socketClient) << std::endl;*/

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

