#pragma once
//출처: 강정중님의 저서 '온라인 게임서버'에서

#include <iostream>
#include <vector>
#include <thread>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

#define MAX_SOCKBUF 1024    //패킷 크기
#define MAX_WORKERTHREAD 4  //쓰레드 풀에 넣을 쓰레드 수

enum class IOOperation
{
    RECV,
    SEND
};

//WSAOVERLAPPED구조체를 확장 시켜서 필요한 정보를 더 넣었다.
struct stOverlappedEx
{
    WSAOVERLAPPED m_wsaOverlapped;    //Overlapped I/O구조체
    SOCKET        m_socketClient;     //클라이언트 소켓
    WSABUF        m_wsaBuf;           //Overlapped I/O작업 버퍼
    char          m_szBuf[MAX_SOCKBUF]; //데이터 버퍼
    IOOperation   m_eOperation;       //작업 동작 종류
};

//클라이언트 정보를 담기위한 구조체
struct stClientInfo
{
    SOCKET          m_socketClient;         //Cliet와 연결되는 소켓
    stOverlappedEx  m_stRecvOverlappedEx;   //RECV Overlapped I/O작업을 위한 변수
    stOverlappedEx  m_stSendOverlappedEx;   //SEND Overlapped I/O작업을 위한 변수

    stClientInfo()
    {
        std::memset(&m_stRecvOverlappedEx, 0, sizeof(stOverlappedEx));
        std::memset(&m_stSendOverlappedEx, 0, sizeof(stOverlappedEx));
        m_socketClient = INVALID_SOCKET;
    }
};

class IOCompletionPort
{
    
    std::vector<stClientInfo> mClientInfos;     //클라이언트 정보 저장 구조체
    SOCKET mListenSocket;                       //클라이언트의 접속을 받기위한 리슨 소켓
    int mClientCnt;                             //접속 되어있는 클라이언트
    std::vector<std::thread> mIOWorkerThreads;  //IO Worker 스레드
    std::thread mAccepterThread;                //Accept 스레드
    HANDLE mIOCPHandle;                         //CompletionPort객체 핸들
    bool mIsWorkerRun;                          //작업 쓰레드 동작 플래그
    bool mIsAccepterRun;                        //접속 쓰레드 동작 플래그
    char mSocketBuf[1024];                      //소켓 버퍼
public:
	IOCompletionPort(): mListenSocket{ INVALID_SOCKET }, mClientCnt{ 0 },
	mIOCPHandle{ INVALID_HANDLE_VALUE }, mIsWorkerRun{ true }, mIsAccepterRun{ true }, mSocketBuf{ 0, }
	{}

    ~IOCompletionPort()
    {
        //윈도우 소켓의 사용을 종료한다.
        WSACleanup();
    }

    //소켓을 초기화하는 함수
    bool InitSocket()
    {
		WSADATA wsaData;//소켓 라이브러리를 저장할 구조체

		//2.2버전의 소켓 라이브러리를 초기화한다.
        //성공하면 0 실패하면 에러코드
        int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        if (0 != nRet)
        {
			//에러가 발생하면 마지막 에러코드를 출력하고 종료한다.
            std::cerr << "WSAStartup() Error : " << WSAGetLastError() << std::endl;
            return false;
        }

        //연결지향형 TCP , Overlapped I/O 소켓을 생성
		//WSASocket(주소체계, 소켓타입, 프로토콜, 프로토콜 정보, 그룹, 플래그) 소켓 생성하는 함수
        /*SOCKET WSASocket(
							int af,	//af (Address Family): 주소 체계를 지정합니다. AF_INET은 IPv4 주소 체계를 의미합니다.
							int type, //type (Socket Type): 소켓의 유형을 지정합니다. SOCK_STREAM은 연결 지향형 소켓(TCP)을 의미합니다.
							int protocol, //사용할 프로토콜 지정 IPPROTO_TCP는 TCP 프로토콜을 의미합니다.
							LPWSAPROTOCOL_INFO lpProtocolInfo, //프로토콜 추가정보 기본 프로토콜을 사용할 경우 NULL로 설정합니다.
							GROUP g, //소켓 그룹을 지정합니다. 기본 그룹을 사용할 경우 0으로 설정합니다.
							DWORD dwFlags //소켓의 특성을 지정하는 플래그입니다. WSA_FLAG_OVERLAPPED는 비동기 I/O 작업을 지원하는 소켓을 생성합니다
							);*/
        mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
		//성공하면 INVALID_SOCKET이 아닌 값이 반환된다.

        if (INVALID_SOCKET == mListenSocket)
        {
            std::cerr << "WSASocket() Error : " << WSAGetLastError() << std::endl;
            return false;
        }

        std::cout << "소켓 연결 성공" << std::endl;
        return true;
    }

    //------서버용 함수-------//
    //서버의 주소정보를 소켓과 연결시키고 접속 요청을 받기 위해 
    //소켓을 등록하는 함수
    bool BindandListen(int nBindPort)
    {
        SOCKADDR_IN stServerAddr;
        stServerAddr.sin_family = AF_INET;
        stServerAddr.sin_port = htons(nBindPort); //서버 포트를 설정한다.        
        //어떤 주소에서 들어오는 접속이라도 받아들이겠다.
        //보통 서버라면 이렇게 설정한다. 만약 한 아이피에서만 접속을 받고 싶다면
        //그 주소를 inet_addr함수를 이용해 넣으면 된다.
        stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        //위에서 지정한 서버 주소 정보와 cIOCompletionPort 소켓을 연결한다.
        int nRet = bind(mListenSocket, reinterpret_cast<SOCKADDR*>(&stServerAddr), sizeof(SOCKADDR_IN));

        if (0 != nRet)
        {
            std::cerr << "[에러] bind()함수 실패 : " << WSAGetLastError() << std::endl;
            return false;
        }

        //접속 요청을 받아들이기 위해 cIOCompletionPort소켓을 등록하고 
        //접속대기큐를 5개로 설정 한다.
        nRet = listen(mListenSocket, 5);
        if (0 != nRet)
        {
            std::cerr << "[에러] listen() 함수 실패 : " << WSAGetLastError() << std::endl;
            return false;
        }

        std::cout << "서버 등록 성공..\n";
        return true;
    }

    //접속 요청을 수락하고 메세지를 받아서 처리하는 함수
    bool StartServer(const UINT32 maxClientCount)
    {
        CreateClient(maxClientCount);

        mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, MAX_WORKERTHREAD);
        if (nullptr == mIOCPHandle)
        {
            std::cerr << "[에러] CreateIoCompletionPort()함수 실패: " << GetLastError() << std::endl;
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

        std::cout << "서버 시작..\n";
        return true;
    }

    //생성되어있는 쓰레드를 파괴한다.
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

        //Accepter 쓰레드를 종료한다.
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

    //WaitingThread Queue에서 대기할 쓰레드들을 생성
    bool CreateWokerThread()
    {
        //WaingThread Queue에 대기 상태로 넣을 쓰레드들 생성 권장되는 개수 : (cpu개수 * 2) + 1 
        for (int i = 0; i < MAX_WORKERTHREAD; i++)
        {
            mIOWorkerThreads.emplace_back([this]() { WokerThread(); });
        }

        std::cout << "WokerThread 시작..\n";
        return true;
    }

    //accept요청을 처리하는 쓰레드 생성
    bool CreateAccepterThread()
    {
        mAccepterThread = std::thread([this]() { AccepterThread(); });

        std::cout << "AccepterThread 시작..\n";
        return true;
    }

    //사용하지 않는 클라이언트 정보 구조체를 반환한다.
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

    //CompletionPort객체와 소켓과 CompletionKey를 연결시키는 역할을 한다.
    bool BindIOCompletionPort(stClientInfo* pClientInfo)
    {
        //socket과 pClientInfo를 CompletionPort객체와 연결시킨다.
        auto hIOCP = CreateIoCompletionPort((HANDLE)pClientInfo->m_socketClient,
                                            mIOCPHandle,
                                            (ULONG_PTR)(pClientInfo), 0);

        if (nullptr == hIOCP || mIOCPHandle != hIOCP)
        {
            std::cerr << "[에러] CreateIoCompletionPort()함수 실패: " << GetLastError() << std::endl;
            return false;
        }

        return true;
    }

    //WSARecv Overlapped I/O 작업을 시킨다.
    bool BindRecv(stClientInfo* pClientInfo)
    {
        DWORD dwFlag = 0;
        DWORD dwRecvNumBytes = 0;

        //Overlapped I/O을 위해 각 정보를 셋팅해 준다.
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

        //socket_error이면 client socket이 끊어진걸로 처리한다.
        if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
        {
            std::cerr << "[에러] WSARecv()함수 실패 : " << WSAGetLastError() << std::endl;
            return false;
        }

        return true;
    }

    //WSASend Overlapped I/O작업을 시킨다.
    bool SendMsg(stClientInfo* pClientInfo, char* pMsg, int nLen)
    {
        DWORD dwRecvNumBytes = 0;

        //전송될 메세지를 복사
        std::memcpy(pClientInfo->m_stSendOverlappedEx.m_szBuf, pMsg, nLen);

        //Overlapped I/O을 위해 각 정보를 셋팅해 준다.
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

        //socket_error이면 client socket이 끊어진걸로 처리한다.
        if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
        {
            std::cerr << "[에러] WSASend()함수 실패 : " << WSAGetLastError() << std::endl;
            return false;
        }
        return true;
    }

    //Overlapped I/O작업에 대한 완료 통보를 받아 
    //그에 해당하는 처리를 하는 함수
    void WokerThread()
    {
        //CompletionKey를 받을 포인터 변수
        stClientInfo* pClientInfo = nullptr;
        //함수 호출 성공 여부
        BOOL bSuccess = TRUE;
        //Overlapped I/O작업에서 전송된 데이터 크기
        DWORD dwIoSize = 0;
        //I/O 작업을 위해 요청한 Overlapped 구조체를 받을 포인터
        LPOVERLAPPED lpOverlapped = nullptr;

        while (mIsWorkerRun)
        {
            //////////////////////////////////////////////////////
            //이 함수로 인해 쓰레드들은 WaitingThread Queue에
            //대기 상태로 들어가게 된다.
            //완료된 Overlapped I/O작업이 발생하면 IOCP Queue에서
            //완료된 작업을 가져와 뒤 처리를 한다.
            //그리고 PostQueuedCompletionStatus()함수에의해 사용자
            //메세지가 도착되면 쓰레드를 종료한다.
            //////////////////////////////////////////////////////
            bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
                                                 &dwIoSize,                    // 실제로 전송된 바이트
                                                 (PULONG_PTR)&pClientInfo,     // CompletionKey
                                                 &lpOverlapped,                // Overlapped IO 객체
                                                 INFINITE);       // 대기할 시간

            //사용자 쓰레드 종료 메세지 처리..
            if (TRUE == bSuccess && 0 == dwIoSize && nullptr == lpOverlapped)
            {
                mIsWorkerRun = false;
                continue;
            }

            if (nullptr == lpOverlapped)
            {
                continue;
            }

            //client가 접속을 끊었을때..            
            if (FALSE == bSuccess || (0 == dwIoSize && TRUE == bSuccess))
            {
                std::cout << "socket(" << static_cast<int>(pClientInfo->m_socketClient) << ") 접속 끊김\n";
                CloseSocket(pClientInfo);
                continue;
            }

            stOverlappedEx* pOverlappedEx = reinterpret_cast<stOverlappedEx*>(lpOverlapped);

            //Overlapped I/O Recv작업 결과 뒤 처리
            if (IOOperation::RECV == pOverlappedEx->m_eOperation)
            {
                pOverlappedEx->m_szBuf[dwIoSize] = '\0';
                std::cout << "[수신] bytes : " << dwIoSize << " , msg : " << pOverlappedEx->m_szBuf << std::endl;

                //클라이언트에 메세지를 에코한다.
                SendMsg(pClientInfo, pOverlappedEx->m_szBuf, dwIoSize);
                BindRecv(pClientInfo);
            }
            //Overlapped I/O Send작업 결과 뒤 처리
            else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
            {
                std::cout << "[송신] bytes : " << dwIoSize << " , msg : " << pOverlappedEx->m_szBuf << std::endl;
            }
            //예외 상황
            else
            {
                std::cout << "socket(" << static_cast<int>(pClientInfo->m_socketClient) << ")에서 예외상황\n";
            }
        }
    }

    //사용자의 접속을 받는 쓰레드
    void AccepterThread()
    {
        SOCKADDR_IN stClientAddr;
        int nAddrLen = sizeof(SOCKADDR_IN);

        while (mIsAccepterRun)
        {
            //접속을 받을 구조체의 인덱스를 얻어온다.
            stClientInfo* pClientInfo = GetEmptyClientInfo();
            if (nullptr == pClientInfo)
            {
                std::cerr << "[에러] Client Full\n";
                return;
            }

            //클라이언트 접속 요청이 들어올 때까지 기다린다.
            pClientInfo->m_socketClient = accept(mListenSocket, reinterpret_cast<SOCKADDR*>(&stClientAddr), &nAddrLen);
            if (INVALID_SOCKET == pClientInfo->m_socketClient)
            {
                continue;
            }

            //I/O Completion Port객체와 소켓을 연결시킨다.
            bool bRet = BindIOCompletionPort(pClientInfo);
            if (!bRet)
            {
                return;
            }

            //Recv Overlapped I/O작업을 요청해 놓는다.
            bRet = BindRecv(pClientInfo);
            if (!bRet)
            {
                return;
            }

            char clientIP[32] = { 0, };
            inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
            std::cout << "클라이언트 접속 : IP(" << clientIP << ") SOCKET(" << static_cast<int>(pClientInfo->m_socketClient) << ")\n";

            //클라이언트 갯수 증가
            ++mClientCnt;
        }
    }

    //소켓의 연결을 종료 시킨다.
    void CloseSocket(stClientInfo* pClientInfo, bool bIsForce = false)
    {
        struct linger stLinger = { 0, 0 };    // SO_DONTLINGER로 설정

        // bIsForce가 true이면 SO_LINGER, timeout = 0으로 설정하여 강제 종료 시킨다. 주의 : 데이터 손실이 있을수 있음 
        if (true == bIsForce)
        {
            stLinger.l_onoff = 1;
        }

        //socketClose소켓의 데이터 송수신을 모두 중단 시킨다.
        shutdown(pClientInfo->m_socketClient, SD_BOTH);

        //소켓 옵션을 설정한다.
        setsockopt(pClientInfo->m_socketClient, SOL_SOCKET, SO_LINGER, reinterpret_cast<char*>(&stLinger), sizeof(stLinger));

        //소켓 연결을 종료 시킨다. 
        closesocket(pClientInfo->m_socketClient);

        pClientInfo->m_socketClient = INVALID_SOCKET;
    }

};
