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

    // Winsock �ʱ�ȭ
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup ����: " << WSAGetLastError() << std::endl;
        return 1;
    }

    // ���� ����
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "���� ���� ����: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // ���� �ּ� ����
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    // ������ ����
    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "������ ���� ����: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "������ ���� ����!" << std::endl;

    // ������ �޽��� ����
    const char* sendMsg = "Hello, Server!";
    send(clientSocket, sendMsg, strlen(sendMsg), 0);

    // �����κ��� �޽��� ����
    char recvBuf[1024] = { 0 };
    int recvLen = recv(clientSocket, recvBuf, sizeof(recvBuf) - 1, 0);
    if (recvLen > 0)
    {
        std::cout << "�����κ��� ������ �޽���: " << recvBuf << std::endl;
    }

    // ���� �ݱ�
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
