#include "pch.h"
#include "EchoServer.h"
#include "Define.h"
constexpr UINT16 SERVER_PORT = 11021;
constexpr UINT16 MAX_CLIENT = 3;		//�� �����Ҽ� �ִ� Ŭ���̾�Ʈ ��
constexpr UINT32 MAX_IO_WORKER = 4;		//IO Worker ������ ��




//
// ä�� ������ ����� ����.
// 1. EchoServer�� ����� ä�� ������ �����
// 2. Packet.h�� ä�ÿ� ��Ŷ�� �߰��Ѵ�. + ��Ŷ����
// 3. ��Ŷ �Ŵ����� ��Ŷ ó��
// 4. ���� �ڵ� �̸�����
// 5. ���� ������ ������ �� �ִ� ����ü�� ���� ����� ������ �����Ѵ�.
// 6. ���� �Ŵ����� ���� ����
// 7. ���� �Ҽ� �ִ� Ŭ���̾�Ʈ�� �����غ����� ���������� ����׿� Ŭ���̾�Ʈ�� �������� ���ҵ�
//
int main()
{
	EchoServer server;

	server.Init(MAX_IO_WORKER);

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