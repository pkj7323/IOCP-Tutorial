#include "IOCP.h"

const std::uint16_t SERVER_PORT = 11021;
const std::uint16_t MAX_CLIENT = 100;		//총 접속할수 있는 클라이언트 수

int main()
{
	IOCP ioCompletionPort;

	//소켓을 초기화
	ioCompletionPort.InitSocket();

	//소켓과 서버 주소를 연결하고 등록 시킨다.
	ioCompletionPort.BindandListen(SERVER_PORT);

	ioCompletionPort.StartServer(MAX_CLIENT);

	printf("아무 키나 누를 때까지 대기합니다\n");
	getchar();

	ioCompletionPort.DestroyThread();

	return 0;
}