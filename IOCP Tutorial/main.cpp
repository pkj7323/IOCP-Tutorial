#include "pch.h"
#include "EchoServer.h"
#include "Define.h"
constexpr UINT16 SERVER_PORT = 11021;
constexpr UINT16 MAX_CLIENT = 3;		//총 접속할수 있는 클라이언트 수
constexpr UINT32 MAX_IO_WORKER = 4;		//IO Worker 스레드 수




//
// 채팅 서버를 만들어 보자.
// 1. EchoServer와 비슷한 채팅 서버를 만들기
// 2. Packet.h에 채팅용 패킷을 추가한다. + 패킷정의
// 3. 패킷 매니저로 패킷 처리
// 4. 에러 코드 미리정의
// 5. 유저 정보를 저장할 수 있는 구조체를 만들어서 사용자 정보를 저장한다.
// 6. 유저 매니저로 유저 관리
// 7. 접속 할수 있는 클라이언트는 생각해봐야함 원래까지의 디버그용 클라이언트로 접속하지 못할듯
//
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