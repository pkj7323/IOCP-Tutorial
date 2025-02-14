#include "EchoServer.h"
#include "Define.h"
const std::uint16_t SERVER_PORT = 11021;
const std::uint16_t MAX_CLIENT = 100;		//총 접속할수 있는 클라이언트 수

int main()
{
	EchoServer server;

	server.InitSocket();

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