#include "pch.h"
#include "EchoServer.h"
#include "Define.h"
constexpr UINT16 SERVER_PORT = 11021;
constexpr UINT16 MAX_CLIENT = 3;		//총 접속할수 있는 클라이언트 수
constexpr UINT32 MAX_IO_WORKER = 4;		//IO Worker 스레드 수
int main()
{
	EchoServer server;

	server.Init(MAX_IO_WORKER);

	server.BindandListen(SERVER_PORT);

	server.Run(MAX_CLIENT);

	std::cout << "아무키나 누를때까지 대기\n";
	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);

		if (inputCmd == "quit")
		{
			break;
		}
	}

	server.End();

	return 0;
}