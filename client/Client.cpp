#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

const std::uint16_t SERVER_PORT = 11021;
const char* SERVER_IP = "127.0.0.1";

int main()
{
    WSADATA     wsaData;
    SOCKET      clientSocket = INVALID_SOCKET;
    sockaddr_in serverAddr;

    // Winsock 초기화
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup 실패: " << WSAGetLastError() << std::endl;
        return 1;
    }

    // 소켓 생성
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "소켓 생성 실패: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // 서버 주소 설정
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    // 서버에 연결
    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "서버에 연결 실패: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "서버에 연결 성공!" << std::endl;

    // 서버로 메시지 전송
    const char* sendMsg = "Hello, Server!";
    send(clientSocket, sendMsg, strlen(sendMsg), 0);

    // 서버로부터 메시지 수신
    char recvBuf[1024] = { 0 };
    int recvLen = recv(clientSocket, recvBuf, sizeof(recvBuf) - 1, 0);
    if (recvLen > 0)
    {
        std::cout << "서버로부터 수신한 메시지: " << recvBuf << std::endl;
    }

    // 소켓 닫기
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
