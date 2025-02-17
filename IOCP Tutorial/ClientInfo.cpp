#include "pch.h"
#include "ClientInfo.h"

ClientInfo::ClientInfo()
	: m_nIndex{ -1 }, m_socketClient{ INVALID_SOCKET }
{
	ZeroMemory(&m_RecvOverlappedEx, sizeof(OverlappedEx));
}


void ClientInfo::Init(const UINT32& index)
{
	m_nIndex = index;
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

	closesocket(m_socketClient);
	m_socketClient = INVALID_SOCKET;
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
	m_RecvOverlappedEx.m_eOperation = IOOperation::recv;

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
	sendOverlappedEx->m_eOperation = IOOperation::send;

	std::lock_guard<std::mutex> lock(m_Lock);
	m_sendDataQ.push(std::move(sendOverlappedEx));

	if (m_sendDataQ.size() == 1)
	{
		SendIO();
	}

	return true;
}

void ClientInfo::SendCompleted(const UINT32& dataSize_)
{
	std::cout << "[송신 완료] bytes : " << dataSize_ << std::endl;

	std::lock_guard<std::mutex> lock(m_Lock);
	m_sendDataQ.pop();

	if (m_sendDataQ.empty() == false)
	{
		SendIO();
	}
}

bool ClientInfo::SendIO()
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
