#include "EchoServer.h"
#include "Define.h"
constexpr UINT16 SERVER_PORT = 11021;
constexpr UINT16 MAX_CLIENT = 100;		//�� �����Ҽ� �ִ� Ŭ���̾�Ʈ ��

int main()
{
	EchoServer server;

	server.InitSocket();

	server.BindandListen(SERVER_PORT);

	server.Run(MAX_CLIENT);

	std::cout << "�ƹ�Ű�� ���������� ���\n";
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