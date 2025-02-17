#pragma once


constexpr UINT32 MAX_SOCKBUF = 256;
constexpr UINT32 MAX_SOCK_SENDBUF = 4096;
constexpr UINT64 RE_USE_SESSION_WAIT_TIMESEC = 3;


enum class IOOperation
{
	RECV,
	SEND,
	ACCEPT
};

//overlapped I/O를 위해 구조체를 확장시킨다.
struct OverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;
	UINT32        m_sessionIndex = 0;
	WSABUF        m_wsaBuf;
	IOOperation   m_eOperation;
	~OverlappedEx()
	{
		delete[] m_wsaBuf.buf;
	}
};

