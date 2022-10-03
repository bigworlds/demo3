#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <WinSock2.h>
#include <MSWsock.h>
#include <WS2tcpip.h>
#include <iostream>
#include <deque>
#include <process.h>
#include <cassert>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

const unsigned short PORT = 5051;
const DWORD RECV_BUFFER_SIZE = 1024;
const DWORD SEND_BUFFER_SIZE = 1024;

struct Session
{
	WSAOVERLAPPED overlpped = {};
	char pBuffer[RECV_BUFFER_SIZE] = {};
	SOCKET hSockClient;
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

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	//getaddrinfo();

	if (SOCKET_ERROR == bind(hSockListening, (struct sockaddr*)&addr, sizeof(addr)))
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
	
	Session* session = new Session();

	HANDLE ret1 = ::CreateIoCompletionPort((HANDLE)hSockListening, hIOCP, (ULONG_PTR)session, 0);
	if (ret1 != hIOCP)
	{
		printf_s("CreateIoCompletionPort Error: %d\n", GetLastError());
		exit(0);
	}

	DWORD recvBytes = 0;
	DWORD zero_byte_read_flags = MSG_PEEK;
	WSABUF zero_byte_read_buffer;
	zero_byte_read_buffer.buf = 0;
	zero_byte_read_buffer.len = 0;
	int ret2 = WSARecv(hSockListening, &zero_byte_read_buffer, 1, &recvBytes, &zero_byte_read_flags, &session->overlpped, NULL);
	if (ret2 == SOCKET_ERROR)
	{
		printf_s("WSARecv Error: %d\n", GetLastError());
		//WSA_IO_PENDING = 997
	}

	while(1)
	{
		Session* pse = (Session* )alloca(sizeof(Session));;
		DWORD bytesTransfered = 0;
		bool ret = GetQueuedCompletionStatus(hIOCP, &bytesTransfered, (PULONG_PTR)&pse, (LPOVERLAPPED *)&pse->overlpped, INFINITE);
		if (ret == FALSE)
		{
			DWORD err234 = GetLastError();
			if (err234 == ERROR_MORE_DATA)
			{
				printf_s("Good to go\n");

				//6. obtain client socket addr
				//WSARecvFrom()
			}
			else
			{
				printf_s("GQCS Error: %d\n", GetLastError());
			}
		}
		
		ret2 = WSARecv(hSockListening, &zero_byte_read_buffer, 1, &recvBytes, &zero_byte_read_flags, &session->overlpped, NULL);

		Sleep(1000 * 2);
	}

	return 0;
}
