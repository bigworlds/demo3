#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <WinSock2.h>
#include <MSWsock.h>
#include <WS2tcpip.h>
#include <iostream>
#include <deque>
#include <process.h>
#include <cassert>
#include <unordered_map>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

const unsigned short PORT = 5051;
const DWORD RECV_BUFFER_SIZE = 1024;
const DWORD SEND_BUFFER_SIZE = 1024;

struct Session
{
	WSAOVERLAPPED overlpped = {};
	SOCKET hSockClient;
	sockaddr_in addr;
	char pBuffer[RECV_BUFFER_SIZE] = {};
};

int main()
{
	WSADATA data;
	if (0 != ::WSAStartup(MAKEWORD(2, 2), &data))
	{
		printf_s("WSAStartup Error: %d\n", GetLastError());
		exit(0);
	}

	SOCKET hSockListening = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (hSockListening == INVALID_SOCKET)
	{
		printf_s("WSASocket Error: %d\n", GetLastError());
		exit(0);
	}

	char optval = 1;
	int ret0 = setsockopt(hSockListening, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(char));
	if (ret0 == SOCKET_ERROR)
	{
		printf_s("setsockopt Error: %d\n", GetLastError());
		exit(0);
	}

	sockaddr_in addrListening = {};
	//getaddrinfo();
	addrListening.sin_family = AF_INET;
	addrListening.sin_port = htons(PORT);
	addrListening.sin_addr.s_addr = INADDR_ANY;
	
	if (SOCKET_ERROR == bind(hSockListening, (struct sockaddr*)&addrListening, sizeof(addrListening)))
	{
		printf_s("bind Error: %d\n", GetLastError());
		exit(0);
	}

	HANDLE hIOCP = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	if (NULL == hIOCP)
	{
		printf_s("CreateIoCompletionPort Error: %d\n", GetLastError());
		exit(0);
	}
	
	Session* sessionListening = new Session();

	HANDLE ret1 = ::CreateIoCompletionPort((HANDLE)hSockListening, hIOCP, (ULONG_PTR)sessionListening, 0);
	if (ret1 != hIOCP)
	{
		printf_s("CreateIoCompletionPort Error: %d\n", GetLastError());
		exit(0);
	}

	//4. Use WsaRecv() to create a single zero-byte overlapped operation for each I/O thread to initiate communications.  
	//You will need to include the MSG_PEEK flag.
	DWORD recvBytes = 0;
	DWORD zero_byte_read_flags = MSG_PEEK;
	WSABUF zero_byte_read_buffer;
	zero_byte_read_buffer.buf = 0;
	zero_byte_read_buffer.len = 0;
	int ret2 = WSARecv(hSockListening, &zero_byte_read_buffer, 1, &recvBytes, &zero_byte_read_flags, &sessionListening->overlpped, NULL);
	if (ret2 == SOCKET_ERROR)
	{
		//printf_s("Prime-WSARecv Error: %d\n", GetLastError());
		//WSA_IO_PENDING = 997
	}

	Session* pSessionListening = new(Session);
	std::unordered_map<int, int> pid2sessionidmap;
	std::vector<Session> sessionList;
	while(1)
	{
		DWORD bytesTransfered = 0;
		bool ret = GetQueuedCompletionStatus(hIOCP, &bytesTransfered, (PULONG_PTR)&pSessionListening, (LPOVERLAPPED *)&pSessionListening->overlpped, INFINITE);
		if (ret == FALSE)
		{
			DWORD err234 = GetLastError();
			if (err234 == ERROR_MORE_DATA)
			{
				printf_s("ERROR_MORE_DATA\n");
			}
			else
			{
				printf_s("GQCS Error: %d\n", err234);
			}
		}
		
		//6. obtain client socket addr
		DWORD recvBytes = 0;
		DWORD cli_peek_flags = MSG_PEEK; //확인만 하고 pop안함 클라이언트 코드에서 켜보면 뭔소린지 알거임
		char pBuffer[RECV_BUFFER_SIZE] = {};
		WSABUF cli_buffer;
		cli_buffer.buf = pBuffer;
		cli_buffer.len = RECV_BUFFER_SIZE;
		sockaddr_in client_addr;
		int addr_size = sizeof(client_addr);
		int res3 = WSARecvFrom(hSockListening, &cli_buffer, 1, &recvBytes, &cli_peek_flags, (struct sockaddr*)&client_addr, &addr_size, NULL, NULL);

		if (res3 != ERROR_SUCCESS)
		{
			int err997 = GetLastError();
			if (err997 != WSA_IO_PENDING)
			{
				printf_s("WSARecvFrom Error: %d\n", err997);
			}
		}
		
		//7. 클라이언트 주소를 얻은 다음
		//클라이언트와 세션을 (소켓) 만들어서 거기다 대고 send/recv 해야 할것 같은데?
		//결국 accept하는게 아닌가
		int pid = 0; //주소대신 사용해봄
		memcpy(&pid, cli_buffer.buf, sizeof(int));
		int count = 0;
		memcpy(&count, cli_buffer.buf + sizeof(int), sizeof(int));
		int port = ntohs(client_addr.sin_port);
		printf_s("%d, %d from client port: %d\n", pid, count, port);

		if (pid2sessionidmap.find(pid) == pid2sessionidmap.end())
		{
			//new session 만들기
			Session newcomer = {};
			SOCKET hClientSock = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
			if (hClientSock == INVALID_SOCKET)
			{
				printf_s("WSASocket Error: %d\n", GetLastError());
				//exit(0);
			}

			char optval = 1;
			int ret0 = setsockopt(hClientSock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(char));
			if (ret0 == SOCKET_ERROR)
			{
				printf_s("setsockopt Error: %d\n", GetLastError());
				//exit(0);
			}

			//3. Bind() the client socket to the socket address of the listening socket.
			//쫌 이상한 방식. 리스닝소켓에다 선요청하는것을 같이 쓰려고 하는것 같은데 
			//This does not work the same as Linux, more on this later.
			//역시나
			if (SOCKET_ERROR == bind(hClientSock, (struct sockaddr*)&addrListening, sizeof(addrListening)))
			{
				printf_s("bind Error: %d\n", GetLastError());
				//exit(0);
			}

			//4. Connect() the client socket to client socket address.  
			//This is the socket address received in the WsaRecvFrom() method, not the listening socket address.
			newcomer.addr.sin_family = AF_INET;
			newcomer.addr.sin_port = (client_addr.sin_port);
			newcomer.addr.sin_addr = (client_addr.sin_addr);
			if (SOCKET_ERROR == connect(hClientSock, (struct sockaddr*)&newcomer.addr, sizeof(newcomer.addr)))
			{
				printf_s("connect Error: %d\n", GetLastError());
				//exit(0);
			}

			newcomer.hSockClient = hClientSock;
			int newsessionid = sessionList.size();
			pid2sessionidmap[pid] = newsessionid;
			sessionList.push_back(newcomer);

			HANDLE ret1 = ::CreateIoCompletionPort((HANDLE)hClientSock, hIOCP, (ULONG_PTR)&sessionList[newsessionid].overlpped, 0);
			if (ret1 != hIOCP)
			{
				printf_s("CreateIoCompletionPort Error: %d\n", GetLastError());
				//exit(0);
			}
		}
		else
		{
			//일단은 pong
			Session& session = sessionList[pid2sessionidmap[pid]];
			DWORD sendBytes = SEND_BUFFER_SIZE;
			WSABUF send_buffer;
			send_buffer.buf = session.pBuffer;
			send_buffer.len = SEND_BUFFER_SIZE;
			memcpy(session.pBuffer, cli_buffer.buf, SEND_BUFFER_SIZE);

			int res4 = WSASend(session.hSockClient, &cli_buffer, 1, &sendBytes, 0, &session.overlpped, NULL);

			if (res4 != ERROR_SUCCESS)
			{
				int err997 = GetLastError();

				if (err997 != WSA_IO_PENDING)
				{
					printf_s("WSASendTo Error: %d\n", err997);
				}
			}
		}
		
		//Prime
		ret2 = WSARecv(hSockListening, &zero_byte_read_buffer, 1, &recvBytes, &zero_byte_read_flags, &sessionListening->overlpped, NULL);
		if (ret2 == SOCKET_ERROR)
		{
			//printf_s("WSARecv Error: %d\n", GetLastError());
			//WSA_IO_PENDING = 997
		}
		//Sleep(1000 * 2);
	}

	return 0;
}
