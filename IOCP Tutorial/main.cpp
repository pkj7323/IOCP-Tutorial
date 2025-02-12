#include "EchoServer.h"
#include "Define.h"
const std::uint16_t SERVER_PORT = 11021;
const std::uint16_t MAX_CLIENT = 100;		//�� �����Ҽ� �ִ� Ŭ���̾�Ʈ ��

int main()
{
	EchoServer server;

	server.InitSocket();

	server.StartServer(MAX_CLIENT);

	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);
		if (inputCmd == "quit")
		{
			break;
		}
	}

	server.DestroyThread();

	return 0;
}