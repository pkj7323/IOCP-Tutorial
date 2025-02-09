#pragma once
//��ó: �����ߴ��� ���� '�¶��� ���Ӽ���'����

#include <iostream>
#include <vector>
#include <thread>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

#define MAX_SOCKBUF 1024    //��Ŷ ũ��
#define MAX_WORKERTHREAD 4  //������ Ǯ�� ���� ������ ��

enum class IOOperation
{
    RECV,
    SEND
};

//WSAOVERLAPPED����ü�� Ȯ�� ���Ѽ� �ʿ��� ������ �� �־���.
struct stOverlappedEx
{
    WSAOVERLAPPED m_wsaOverlapped;    //Overlapped I/O����ü
    SOCKET        m_socketClient;     //Ŭ���̾�Ʈ ����
    WSABUF        m_wsaBuf;           //Overlapped I/O�۾� ����
    char          m_szBuf[MAX_SOCKBUF]; //������ ����
    IOOperation   m_eOperation;       //�۾� ���� ����
};

//Ŭ���̾�Ʈ ������ ������� ����ü
struct stClientInfo
{
    SOCKET          m_socketClient;         //Cliet�� ����Ǵ� ����
    stOverlappedEx  m_stRecvOverlappedEx;   //RECV Overlapped I/O�۾��� ���� ����
    stOverlappedEx  m_stSendOverlappedEx;   //SEND Overlapped I/O�۾��� ���� ����

    stClientInfo()
    {
        std::memset(&m_stRecvOverlappedEx, 0, sizeof(stOverlappedEx));
        std::memset(&m_stSendOverlappedEx, 0, sizeof(stOverlappedEx));
        m_socketClient = INVALID_SOCKET;
    }
};

class IOCompletionPort
{
    
    std::vector<stClientInfo> mClientInfos;     //Ŭ���̾�Ʈ ���� ���� ����ü
    SOCKET mListenSocket;                       //Ŭ���̾�Ʈ�� ������ �ޱ����� ���� ����
    int mClientCnt;                             //���� �Ǿ��ִ� Ŭ���̾�Ʈ
    std::vector<std::thread> mIOWorkerThreads;  //IO Worker ������
    std::thread mAccepterThread;                //Accept ������
    HANDLE mIOCPHandle;                         //CompletionPort��ü �ڵ�
    bool mIsWorkerRun;                          //�۾� ������ ���� �÷���
    bool mIsAccepterRun;                        //���� ������ ���� �÷���
    char mSocketBuf[1024];                      //���� ����
public:
	IOCompletionPort(): mListenSocket{ INVALID_SOCKET }, mClientCnt{ 0 },
	mIOCPHandle{ INVALID_HANDLE_VALUE }, mIsWorkerRun{ true }, mIsAccepterRun{ true }, mSocketBuf{ 0, }
	{}

    ~IOCompletionPort()
    {
        //������ ������ ����� �����Ѵ�.
        WSACleanup();
    }

    //������ �ʱ�ȭ�ϴ� �Լ�
    bool InitSocket()
    {
		WSADATA wsaData;//���� ���̺귯���� ������ ����ü

		//2.2������ ���� ���̺귯���� �ʱ�ȭ�Ѵ�.
        //�����ϸ� 0 �����ϸ� �����ڵ�
        int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        if (0 != nRet)
        {
			//������ �߻��ϸ� ������ �����ڵ带 ����ϰ� �����Ѵ�.
            std::cerr << "WSAStartup() Error : " << WSAGetLastError() << std::endl;
            return false;
        }

        //���������� TCP , Overlapped I/O ������ ����
		//WSASocket(�ּ�ü��, ����Ÿ��, ��������, �������� ����, �׷�, �÷���) ���� �����ϴ� �Լ�
        /*SOCKET WSASocket(
							int af,	//af (Address Family): �ּ� ü�踦 �����մϴ�. AF_INET�� IPv4 �ּ� ü�踦 �ǹ��մϴ�.
							int type, //type (Socket Type): ������ ������ �����մϴ�. SOCK_STREAM�� ���� ������ ����(TCP)�� �ǹ��մϴ�.
							int protocol, //����� �������� ���� IPPROTO_TCP�� TCP ���������� �ǹ��մϴ�.
							LPWSAPROTOCOL_INFO lpProtocolInfo, //�������� �߰����� �⺻ ���������� ����� ��� NULL�� �����մϴ�.
							GROUP g, //���� �׷��� �����մϴ�. �⺻ �׷��� ����� ��� 0���� �����մϴ�.
							DWORD dwFlags //������ Ư���� �����ϴ� �÷����Դϴ�. WSA_FLAG_OVERLAPPED�� �񵿱� I/O �۾��� �����ϴ� ������ �����մϴ�
							);*/
        mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
		//�����ϸ� INVALID_SOCKET�� �ƴ� ���� ��ȯ�ȴ�.

        if (INVALID_SOCKET == mListenSocket)
        {
            std::cerr << "WSASocket() Error : " << WSAGetLastError() << std::endl;
            return false;
        }

        std::cout << "���� ���� ����" << std::endl;
        return true;
    }

    //------������ �Լ�-------//
    //������ �ּ������� ���ϰ� �����Ű�� ���� ��û�� �ޱ� ���� 
    //������ ����ϴ� �Լ�
    bool BindandListen(int nBindPort)
    {
        SOCKADDR_IN stServerAddr;
        stServerAddr.sin_family = AF_INET;
        stServerAddr.sin_port = htons(nBindPort); //���� ��Ʈ�� �����Ѵ�.        
        //� �ּҿ��� ������ �����̶� �޾Ƶ��̰ڴ�.
        //���� ������� �̷��� �����Ѵ�. ���� �� �����ǿ����� ������ �ް� �ʹٸ�
        //�� �ּҸ� inet_addr�Լ��� �̿��� ������ �ȴ�.
        stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        //������ ������ ���� �ּ� ������ cIOCompletionPort ������ �����Ѵ�.
        int nRet = bind(mListenSocket, reinterpret_cast<SOCKADDR*>(&stServerAddr), sizeof(SOCKADDR_IN));

        if (0 != nRet)
        {
            std::cerr << "[����] bind()�Լ� ���� : " << WSAGetLastError() << std::endl;
            return false;
        }

        //���� ��û�� �޾Ƶ��̱� ���� cIOCompletionPort������ ����ϰ� 
        //���Ӵ��ť�� 5���� ���� �Ѵ�.
        nRet = listen(mListenSocket, 5);
        if (0 != nRet)
        {
            std::cerr << "[����] listen() �Լ� ���� : " << WSAGetLastError() << std::endl;
            return false;
        }

        std::cout << "���� ��� ����..\n";
        return true;
    }

    //���� ��û�� �����ϰ� �޼����� �޾Ƽ� ó���ϴ� �Լ�
    bool StartServer(const UINT32 maxClientCount)
    {
        CreateClient(maxClientCount);

        mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, MAX_WORKERTHREAD);
        if (nullptr == mIOCPHandle)
        {
            std::cerr << "[����] CreateIoCompletionPort()�Լ� ����: " << GetLastError() << std::endl;
            return false;
        }

        bool bRet = CreateWokerThread();
        if (!bRet) {
            return false;
        }

        bRet = CreateAccepterThread();
        if (!bRet) {
            return false;
        }

        std::cout << "���� ����..\n";
        return true;
    }

    //�����Ǿ��ִ� �����带 �ı��Ѵ�.
    void DestroyThread()
    {
        mIsWorkerRun = false;
        CloseHandle(mIOCPHandle);

        for (auto& th : mIOWorkerThreads)
        {
            if (th.joinable())
            {
                th.join();
            }
        }

        //Accepter �����带 �����Ѵ�.
        mIsAccepterRun = false;
        closesocket(mListenSocket);

        if (mAccepterThread.joinable())
        {
            mAccepterThread.join();
        }
    }

private:
    void CreateClient(const UINT32 maxClientCount)
    {
        for (UINT32 i = 0; i < maxClientCount; ++i)
        {
            mClientInfos.emplace_back();
        }
    }

    //WaitingThread Queue���� ����� ��������� ����
    bool CreateWokerThread()
    {
        //WaingThread Queue�� ��� ���·� ���� ������� ���� ����Ǵ� ���� : (cpu���� * 2) + 1 
        for (int i = 0; i < MAX_WORKERTHREAD; i++)
        {
            mIOWorkerThreads.emplace_back([this]() { WokerThread(); });
        }

        std::cout << "WokerThread ����..\n";
        return true;
    }

    //accept��û�� ó���ϴ� ������ ����
    bool CreateAccepterThread()
    {
        mAccepterThread = std::thread([this]() { AccepterThread(); });

        std::cout << "AccepterThread ����..\n";
        return true;
    }

    //������� �ʴ� Ŭ���̾�Ʈ ���� ����ü�� ��ȯ�Ѵ�.
    stClientInfo* GetEmptyClientInfo()
    {
        for (auto& client : mClientInfos)
        {
            if (INVALID_SOCKET == client.m_socketClient)
            {
                return &client;
            }
        }

        return nullptr;
    }

    //CompletionPort��ü�� ���ϰ� CompletionKey�� �����Ű�� ������ �Ѵ�.
    bool BindIOCompletionPort(stClientInfo* pClientInfo)
    {
        //socket�� pClientInfo�� CompletionPort��ü�� �����Ų��.
        auto hIOCP = CreateIoCompletionPort((HANDLE)pClientInfo->m_socketClient,
                                            mIOCPHandle,
                                            (ULONG_PTR)(pClientInfo), 0);

        if (nullptr == hIOCP || mIOCPHandle != hIOCP)
        {
            std::cerr << "[����] CreateIoCompletionPort()�Լ� ����: " << GetLastError() << std::endl;
            return false;
        }

        return true;
    }

    //WSARecv Overlapped I/O �۾��� ��Ų��.
    bool BindRecv(stClientInfo* pClientInfo)
    {
        DWORD dwFlag = 0;
        DWORD dwRecvNumBytes = 0;

        //Overlapped I/O�� ���� �� ������ ������ �ش�.
        pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
        pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.buf = pClientInfo->m_stRecvOverlappedEx.m_szBuf;
        pClientInfo->m_stRecvOverlappedEx.m_eOperation = IOOperation::RECV;

        int nRet = WSARecv(pClientInfo->m_socketClient,
                           &(pClientInfo->m_stRecvOverlappedEx.m_wsaBuf),
                           1,
                           &dwRecvNumBytes,
                           &dwFlag,
                           (LPWSAOVERLAPPED) & (pClientInfo->m_stRecvOverlappedEx),
                           nullptr);

        //socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
        if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
        {
            std::cerr << "[����] WSARecv()�Լ� ���� : " << WSAGetLastError() << std::endl;
            return false;
        }

        return true;
    }

    //WSASend Overlapped I/O�۾��� ��Ų��.
    bool SendMsg(stClientInfo* pClientInfo, char* pMsg, int nLen)
    {
        DWORD dwRecvNumBytes = 0;

        //���۵� �޼����� ����
        std::memcpy(pClientInfo->m_stSendOverlappedEx.m_szBuf, pMsg, nLen);

        //Overlapped I/O�� ���� �� ������ ������ �ش�.
        pClientInfo->m_stSendOverlappedEx.m_wsaBuf.len = nLen;
        pClientInfo->m_stSendOverlappedEx.m_wsaBuf.buf = pClientInfo->m_stSendOverlappedEx.m_szBuf;
        pClientInfo->m_stSendOverlappedEx.m_eOperation = IOOperation::SEND;

        int nRet = WSASend(pClientInfo->m_socketClient,
                           &(pClientInfo->m_stSendOverlappedEx.m_wsaBuf),
                           1,
                           &dwRecvNumBytes,
                           0,
                           (LPWSAOVERLAPPED) & (pClientInfo->m_stSendOverlappedEx),
                           nullptr);

        //socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
        if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
        {
            std::cerr << "[����] WSASend()�Լ� ���� : " << WSAGetLastError() << std::endl;
            return false;
        }
        return true;
    }

    //Overlapped I/O�۾��� ���� �Ϸ� �뺸�� �޾� 
    //�׿� �ش��ϴ� ó���� �ϴ� �Լ�
    void WokerThread()
    {
        //CompletionKey�� ���� ������ ����
        stClientInfo* pClientInfo = nullptr;
        //�Լ� ȣ�� ���� ����
        BOOL bSuccess = TRUE;
        //Overlapped I/O�۾����� ���۵� ������ ũ��
        DWORD dwIoSize = 0;
        //I/O �۾��� ���� ��û�� Overlapped ����ü�� ���� ������
        LPOVERLAPPED lpOverlapped = nullptr;

        while (mIsWorkerRun)
        {
            //////////////////////////////////////////////////////
            //�� �Լ��� ���� ��������� WaitingThread Queue��
            //��� ���·� ���� �ȴ�.
            //�Ϸ�� Overlapped I/O�۾��� �߻��ϸ� IOCP Queue����
            //�Ϸ�� �۾��� ������ �� ó���� �Ѵ�.
            //�׸��� PostQueuedCompletionStatus()�Լ������� �����
            //�޼����� �����Ǹ� �����带 �����Ѵ�.
            //////////////////////////////////////////////////////
            bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
                                                 &dwIoSize,                    // ������ ���۵� ����Ʈ
                                                 (PULONG_PTR)&pClientInfo,     // CompletionKey
                                                 &lpOverlapped,                // Overlapped IO ��ü
                                                 INFINITE);       // ����� �ð�

            //����� ������ ���� �޼��� ó��..
            if (TRUE == bSuccess && 0 == dwIoSize && nullptr == lpOverlapped)
            {
                mIsWorkerRun = false;
                continue;
            }

            if (nullptr == lpOverlapped)
            {
                continue;
            }

            //client�� ������ ��������..            
            if (FALSE == bSuccess || (0 == dwIoSize && TRUE == bSuccess))
            {
                std::cout << "socket(" << static_cast<int>(pClientInfo->m_socketClient) << ") ���� ����\n";
                CloseSocket(pClientInfo);
                continue;
            }

            stOverlappedEx* pOverlappedEx = reinterpret_cast<stOverlappedEx*>(lpOverlapped);

            //Overlapped I/O Recv�۾� ��� �� ó��
            if (IOOperation::RECV == pOverlappedEx->m_eOperation)
            {
                pOverlappedEx->m_szBuf[dwIoSize] = '\0';
                std::cout << "[����] bytes : " << dwIoSize << " , msg : " << pOverlappedEx->m_szBuf << std::endl;

                //Ŭ���̾�Ʈ�� �޼����� �����Ѵ�.
                SendMsg(pClientInfo, pOverlappedEx->m_szBuf, dwIoSize);
                BindRecv(pClientInfo);
            }
            //Overlapped I/O Send�۾� ��� �� ó��
            else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
            {
                std::cout << "[�۽�] bytes : " << dwIoSize << " , msg : " << pOverlappedEx->m_szBuf << std::endl;
            }
            //���� ��Ȳ
            else
            {
                std::cout << "socket(" << static_cast<int>(pClientInfo->m_socketClient) << ")���� ���ܻ�Ȳ\n";
            }
        }
    }

    //������� ������ �޴� ������
    void AccepterThread()
    {
        SOCKADDR_IN stClientAddr;
        int nAddrLen = sizeof(SOCKADDR_IN);

        while (mIsAccepterRun)
        {
            //������ ���� ����ü�� �ε����� ���´�.
            stClientInfo* pClientInfo = GetEmptyClientInfo();
            if (nullptr == pClientInfo)
            {
                std::cerr << "[����] Client Full\n";
                return;
            }

            //Ŭ���̾�Ʈ ���� ��û�� ���� ������ ��ٸ���.
            pClientInfo->m_socketClient = accept(mListenSocket, reinterpret_cast<SOCKADDR*>(&stClientAddr), &nAddrLen);
            if (INVALID_SOCKET == pClientInfo->m_socketClient)
            {
                continue;
            }

            //I/O Completion Port��ü�� ������ �����Ų��.
            bool bRet = BindIOCompletionPort(pClientInfo);
            if (!bRet)
            {
                return;
            }

            //Recv Overlapped I/O�۾��� ��û�� ���´�.
            bRet = BindRecv(pClientInfo);
            if (!bRet)
            {
                return;
            }

            char clientIP[32] = { 0, };
            inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
            std::cout << "Ŭ���̾�Ʈ ���� : IP(" << clientIP << ") SOCKET(" << static_cast<int>(pClientInfo->m_socketClient) << ")\n";

            //Ŭ���̾�Ʈ ���� ����
            ++mClientCnt;
        }
    }

    //������ ������ ���� ��Ų��.
    void CloseSocket(stClientInfo* pClientInfo, bool bIsForce = false)
    {
        struct linger stLinger = { 0, 0 };    // SO_DONTLINGER�� ����

        // bIsForce�� true�̸� SO_LINGER, timeout = 0���� �����Ͽ� ���� ���� ��Ų��. ���� : ������ �ս��� ������ ���� 
        if (true == bIsForce)
        {
            stLinger.l_onoff = 1;
        }

        //socketClose������ ������ �ۼ����� ��� �ߴ� ��Ų��.
        shutdown(pClientInfo->m_socketClient, SD_BOTH);

        //���� �ɼ��� �����Ѵ�.
        setsockopt(pClientInfo->m_socketClient, SOL_SOCKET, SO_LINGER, reinterpret_cast<char*>(&stLinger), sizeof(stLinger));

        //���� ������ ���� ��Ų��. 
        closesocket(pClientInfo->m_socketClient);

        pClientInfo->m_socketClient = INVALID_SOCKET;
    }

};
