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

	MaxIOWorkerThreadCnt = maxIOWorkerThreadCnt_;

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

	m_IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr,
		NULL, MaxIOWorkerThreadCnt);
	if (m_IOCPHandle == nullptr)
	{
		std::cerr << "[에러] CreateIoCompletionPort()함수 실패 : " << GetLastError() << std::endl;
		return false;
	}
	auto hIOCP = CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_listenSocket), 
		m_IOCPHandle,0, 0);
	if (hIOCP == nullptr)
	{
		std::cerr << "[에러] listen socket IOCP bind 실패: " << WSAGetLastError() << std::endl;
		return false;
	}

	std::cout << "서버 등록 성공..\n";
	return true;
}

bool IOCP::StartServer(const UINT32& maxClientCount)
{
	createClient(maxClientCount);

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

	std::cout << "WokerThread 시작..\n";
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

	std::cout << "AccepterThread 시작..\n";
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
			&dwIoSize, //실제로 전송될 바이트
			reinterpret_cast<PULONG_PTR>(&pClientInfo), //사용자 데이터 completion key
			&lpOverlapped, //완료된 I/O작업의 정보
			INFINITE//대기할 시간
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
			//std::cout << "socket(" << static_cast<int>(pClientInfo->GetIndex()) << ")에서 예외상황\n";
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
			std::cout << "socket(" << static_cast<int>(pClientInfo->GetIndex()) << ")에서 예외상황\n";
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


