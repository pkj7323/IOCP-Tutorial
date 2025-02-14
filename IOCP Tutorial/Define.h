#pragma once
#include <winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "ws2_32")
#include <iostream>


#include <thread>
#include <vector>
#include <array>
#include <windows.h>
#include <string>

static const std::uint32_t MAX_SOCKBUF = 256;
static const std::uint32_t MAX_WORKERTHREAD = 100;

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


struct ClientInfo
{
	int32_t			m_nIndex;
	SOCKET			m_socketClient;//Ŭ���̾�Ʈ�� ����Ǵ� ����
	OverlappedEx	m_RecvOverlappedEx;
	OverlappedEx	m_SendOverlappedEx;

	char			m_RecvBuf[MAX_SOCKBUF];
	char			m_SendBuf[MAX_SOCKBUF];

	ClientInfo()
	{
		ZeroMemory(&m_RecvOverlappedEx, sizeof(OverlappedEx));
		ZeroMemory(&m_SendOverlappedEx, sizeof(OverlappedEx));
		m_socketClient = INVALID_SOCKET;
	}
};