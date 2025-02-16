#pragma once


constexpr UINT32 MAX_SOCKBUF = 256;
constexpr UINT32 MAX_SOCK_SENDBUF = 4096;
constexpr UINT32 MAX_WORKERTHREAD = 4;


enum class IOOperation
{
	recv,
	send
};

//overlapped I/O�� ���� ����ü�� Ȯ���Ų��.
struct OverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;
	SOCKET        m_socketClient;
	WSABUF        m_wsaBuf;
	IOOperation   m_eOperation;
};

