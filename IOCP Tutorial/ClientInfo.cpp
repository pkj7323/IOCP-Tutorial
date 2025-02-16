#include "ClientInfo.h"

ClientInfo::ClientInfo()
{
	m_nIndex = -1;
	m_socketClient = INVALID_SOCKET;
	ZeroMemory(&m_RecvOverlappedEx, sizeof(OverlappedEx));
}

void ClientInfo::Init(const UINT32& index)
{
	m_nIndex = index;
}


void ClientInfo::Clear()
{
	mSend
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

bool ClientInfo::SendMsg(const UINT32& dataSize_, char* pMsg)
{
	auto sendOverlappedEX = new OverlappedEx;
	ZeroMemory(sendOverlappedEX, sizeof(OverlappedEx));
	sendOverlappedEX->m_wsaBuf.len = dataSize_;
	sendOverlappedEX->m_wsaBuf.buf = new char[dataSize_];
	CopyMemory(sendOverlappedEX->m_wsaBuf.buf, pMsg, dataSize_);
	sendOverlappedEX->m_eOperation = IOOperation::send;

	DWORD dwSendNumBytes = 0;
	int ret = WSASend
	(
		m_socketClient,
		&sendOverlappedEX->m_wsaBuf,
		1,
		&dwSendNumBytes,
		0,
		reinterpret_cast<LPWSAOVERLAPPED>(sendOverlappedEX),
		nullptr);


	if (ret == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)
	{
		std::cerr << "[에러] WSASend 함수 실패: " << WSAGetLastError();
		return false;
	}

	return true;
}

void ClientInfo::SendCompleted(const UINT32& dataSize_)
{
	std::cout << "[송신 완료] bytes : " << dataSize_ << std::endl;
}


UINT32 ClientInfo::GetIndex() const
{
	return m_nIndex;
}

SOCKET ClientInfo::GetSocket() const
{
	return m_socketClient;
}

bool ClientInfo::IsConnected() const
{
	return m_socketClient != INVALID_SOCKET;
}

char* ClientInfo::GetRecvBuffer()
{
	return m_RecvBuf;
}