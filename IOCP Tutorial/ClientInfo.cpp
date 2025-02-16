#include "pch.h"
#include "ClientInfo.h"

ClientInfo::ClientInfo()
	: m_nIndex{ -1 }, m_socketClient{ INVALID_SOCKET }, m_IsSending{ false }, m_SendPos{ 0 }
{
	ZeroMemory(&m_RecvOverlappedEx, sizeof(OverlappedEx));
	ZeroMemory(&m_SendOverlappedEx, sizeof(OverlappedEx));
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

	setsockopt(m_socketClient, SOL_SOCKET, SO_LINGER, reinterpret_cast<char*>(&stLinger), sizeof(stLinger));

	closesocket(m_socketClient);

	m_socketClient = INVALID_SOCKET;
}

void ClientInfo::Clear()
{
	m_SendPos = 0;
	m_IsSending = false;
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
	std::lock_guard<std::mutex> lock(m_Lock);
	if (m_SendPos + dataSize_ > MAX_SOCK_SENDBUF)
	{
		m_SendPos = 0;
	}

	auto pSendBuffer = &m_SendBuffer[m_SendPos];

	CopyMemory(pSendBuffer, pMsg, dataSize_);
	m_SendPos += dataSize_;

	return true;
}

bool ClientInfo::SendIO()
{
	if (m_SendPos <= 0 || m_IsSending)
	{
		return true;
	}

	std::lock_guard<std::mutex> lock(m_Lock);

	m_IsSending = true;

	CopyMemory(m_SendingBuffer, &m_SendBuffer[0], m_SendPos);

	//overlapped I/O를 위한 초기화
	m_SendOverlappedEx.m_wsaBuf.len = m_SendPos;
	m_SendOverlappedEx.m_wsaBuf.buf = &m_SendingBuffer[0];
	m_SendOverlappedEx.m_eOperation = IOOperation::send;

	DWORD dwRecvNumBytes = 0;
	int nRet = WSASend
	(
		m_socketClient,
		&m_SendOverlappedEx.m_wsaBuf,
		1,
		&dwRecvNumBytes,
		0,
		reinterpret_cast<LPWSAOVERLAPPED>(&m_SendOverlappedEx),
		nullptr);
	if (nRet == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)
	{
		std::cerr << "[에러] WSASend 함수 실패: " << WSAGetLastError();
		return false;
	}
	m_SendPos = 0;

	return true;
}

void ClientInfo::SendCompleted(const UINT32& dataSize_)
{
	std::cout << "[송신 완료] bytes : " << dataSize_ << std::endl;
}