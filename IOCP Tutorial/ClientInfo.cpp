#include "pch.h"
#include "ClientInfo.h"


ClientInfo::ClientInfo(const UINT32& index, HANDLE iocpHandle_)
	: m_nIndex{ -1 }, m_hIOCP{ INVALID_HANDLE_VALUE }, m_isConnect{ 0 }, m_lastestClosedTimeSec{ 0 },
	m_socketClient{ INVALID_SOCKET }
{
	ZeroMemory(&m_RecvOverlappedEx, sizeof(OverlappedEx));
	m_nIndex = index;
	m_hIOCP = iocpHandle_;
}


void ClientInfo::Init(const UINT32& index, HANDLE iocpHandle_)
{
	m_nIndex = index;
	m_hIOCP = iocpHandle_;
}

bool ClientInfo::OnConnect(HANDLE iocpHandle_, SOCKET socket_)
{
	m_socketClient = socket_;

	Clear();

	if (BindIOCompletionPort(iocpHandle_) == false)
	{
		return false;
	}

	return BindRecv();
}

void ClientInfo::Close(bool bIsForce_)
{
	linger stLinger = { 0,0 };

	if (true == bIsForce_ )
	{
		stLinger.l_onoff = 1;
		stLinger.l_linger = 0;
	}

	shutdown(m_socketClient, SD_BOTH);

	setsockopt(m_socketClient, SOL_SOCKET, SO_LINGER,
		reinterpret_cast<char*>(&stLinger), sizeof(stLinger));
	m_isConnect = 0;
	m_lastestClosedTimeSec = std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
	closesocket(m_socketClient);
	m_socketClient = INVALID_SOCKET;
}

bool ClientInfo::PostAccept(SOCKET listenSocket_, UINT64 curTimeSec_)
{
	std::cout << "PostAccept. Client Index: " << GetIndex() << std::endl;
	m_lastestClosedTimeSec = UINT32_MAX;

	m_socketClient = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_IP,
		nullptr, NULL, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_socketClient)
	{
		std::cerr << "client socket WSASocket error" << GetLastError() << std::endl;
		return false;
	}
	ZeroMemory(&m_AcceptContext, 0, sizeof(OverlappedEx));

	DWORD bytes = 0;
	DWORD flags = 0;

	m_AcceptContext.m_wsaBuf.len = 0;
	m_AcceptContext.m_wsaBuf.buf = nullptr;
	m_AcceptContext.m_eOperation = IOOperation::ACCEPT;
	m_AcceptContext.m_sessionIndex = m_nIndex;

	if (FALSE == AcceptEx(listenSocket_, m_socketClient, m_acceptBuf, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytes,
		reinterpret_cast<LPWSAOVERLAPPED>(&m_AcceptContext)))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			std::cerr << "AcceptEx error: " << WSAGetLastError() << std::endl;
			return false;
		}
	}
	
}

bool ClientInfo::AcceptCompletion()
{
	std::cout << "AcceptCompletion : SessionIndex(" << m_nIndex << ")" << std::endl;

	if (OnConnect(m_hIOCP,m_socketClient) == false)
	{
		std::cerr << "OnConnect error" << std::endl;
		return false;
	}

	SOCKADDR_IN		stClientAddr;
	int nAddrLen = sizeof(SOCKADDR_IN);
	char clientIP[32] = { 0,};
	inet_ntop(AF_INET, &stClientAddr.sin_addr, clientIP, 32 - 1);
	std::cout << "Accept IP : " << clientIP << "SOCKET : " << static_cast<int>(m_socketClient) << std::endl;

	return true;
}



bool ClientInfo::BindIOCompletionPort(HANDLE iocpHandle_)
{
	auto hIOCP = CreateIoCompletionPort(
		reinterpret_cast<HANDLE>(GetSocket()),
		iocpHandle_, 
		reinterpret_cast<ULONG_PTR>(this),
		0);

	if (hIOCP == INVALID_HANDLE_VALUE)
	{
		std::cerr << "[에러] CreateIoCompletionPort 함수 실패: " << GetLastError();
		return false;
	}

	return true;
}

bool ClientInfo::BindRecv()
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	m_RecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
	m_RecvOverlappedEx.m_wsaBuf.buf = m_RecvBuf;
	m_RecvOverlappedEx.m_eOperation = IOOperation::RECV;

	int ret = WSARecv
	(
		m_socketClient,
		&m_RecvOverlappedEx.m_wsaBuf,
		1,
		&dwRecvNumBytes,
		&dwFlag,
		reinterpret_cast<LPWSAOVERLAPPED>(&m_RecvOverlappedEx),
		nullptr);

	if (ret == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)
	{
		std::cerr << "[에러] WSARecv 함수 실패: " << WSAGetLastError();
		return false;
	}

	return true;
}


bool ClientInfo::SendMsg(const UINT32& dataSize_, char* pMsg)
{
	
	auto sendOverlappedEx = std::make_unique<OverlappedEx>();
	std::memset(sendOverlappedEx.get(), 0, sizeof(OverlappedEx));
	sendOverlappedEx->m_wsaBuf.len = dataSize_;
	sendOverlappedEx->m_wsaBuf.buf = new char[dataSize_];
    std::copy_n(pMsg, dataSize_, sendOverlappedEx->m_wsaBuf.buf);
	sendOverlappedEx->m_eOperation = IOOperation::SEND;

	std::lock_guard<std::mutex> lock(m_sendLock);
	m_sendDataQ.push(std::move(sendOverlappedEx));

	if (m_sendDataQ.size() == 1)
	{
		sendIO();
	}

	return true;
}

void ClientInfo::SendCompleted(const UINT32& dataSize_)
{
	std::cout << "[송신 완료] bytes : " << dataSize_ << std::endl;

	std::lock_guard<std::mutex> lock(m_sendLock);
	m_sendDataQ.pop();

	if (m_sendDataQ.empty() == false)
	{
		sendIO();
	}
}

bool ClientInfo::sendIO()
{
	auto sendOverlappedEx = m_sendDataQ.front().get();

	DWORD dwSendNumBytes = 0;
	int nRet = WSASend
	(
		m_socketClient,
		&sendOverlappedEx->m_wsaBuf,
		1,
		&dwSendNumBytes,
		0,
		reinterpret_cast<LPWSAOVERLAPPED>(sendOverlappedEx),
		nullptr);
	if (nRet == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)
	{
		std::cerr << "[에러] WSASend 함수 실패: " << WSAGetLastError() << std::endl;
		return false;
	}
	return true;
}

bool ClientInfo::setSocketOption()
{
	/*if (SOCKET_ERROR == setsockopt(mSock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)GIocpManager->GetListenSocket(), sizeof(SOCKET)))
		{
			printf_s("[DEBUG] SO_UPDATE_ACCEPT_CONTEXT error: %d\n", GetLastError());
			return false;
		}*/
	int opt = 1;

	if (SOCKET_ERROR == setsockopt(m_socketClient,IPPROTO_TCP,TCP_NODELAY,
			reinterpret_cast<const char*>(&opt),sizeof(int)))
	{
		std::cerr << "[Debug] setsockopt 함수 실패, TCP_NODELAY error: " << WSAGetLastError() << std::endl;
		return false;
	}

	opt = 0;
	if (SOCKET_ERROR == setsockopt(m_socketClient, SOL_SOCKET, SO_RCVBUF,
		reinterpret_cast<const char*>(&opt), sizeof(int)))
	{
		std::cerr << "[Debug] setsockopt 함수 실패, SO_RCVBUF change error: " << WSAGetLastError() << std::endl;
		return false;
	}
	return true;
}