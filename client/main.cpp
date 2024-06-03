#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <WinSock2.h>
#include <MSWsock.h>
#include <WS2tcpip.h>
#include <iostream>
#include <process.h>
#include <thread>
#pragma comment(lib, "ws2_32.lib")
//#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)

using namespace std;

const char* pIPAddress = "127.0.0.1";
const unsigned short PORT = 5051;
const DWORD RECV_BUFFER_SIZE = 1024;
const DWORD SEND_BUFFER_SIZE = 1024;

int main()
{
	WSADATA data;
	if (0 != ::WSAStartup(MAKEWORD(2,2), &data))
	{
		printf_s("WSAStartup Error: %d\n", GetLastError());
		exit(0);
	}

	SOCKET hSock = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0);
	if (hSock == INVALID_SOCKET)
	{
		printf_s("WSASocket Error: %d\n", GetLastError());
		exit(0);
	}

	//BOOL bNewBehavior = FALSE;
	//DWORD dwBytesReturned = 0;
	//WSAIoctl(hSock, SIO_UDP_CONNRESET, &bNewBehavior, sizeof bNewBehavior, NULL, 0, &dwBytesReturned, NULL, NULL);
	

	sockaddr_in svraddr;
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(PORT);
	svraddr.sin_addr.s_addr = inet_addr(pIPAddress);

	int pid = _getpid();
	char pBuffer[SEND_BUFFER_SIZE] = {};
	memcpy(pBuffer, &pid, sizeof(int));
	WSABUF buf;
	buf.buf = pBuffer;
	buf.len = SEND_BUFFER_SIZE;

	DWORD bytesSent = 0;
	DWORD flags = 0;
	int count = 1;

	//for connect
	memcpy(pBuffer + sizeof(int), &count, sizeof(int));
	if (SOCKET_ERROR == ::WSASendTo(
		hSock,
		&buf,
		1,
		&bytesSent,
		flags,
		reinterpret_cast<sockaddr*>(&svraddr),
		sizeof(svraddr),
		0,
		0))
	{
		printf_s("WSASendTo Error: %d\n", GetLastError());
		//break;
	}

	while(1)
	{
		memcpy(pBuffer + sizeof(int), &count, sizeof(int));
		if (SOCKET_ERROR == ::WSASendTo(
			hSock,
			&buf,
			1,
			&bytesSent,
			flags,
			reinterpret_cast<sockaddr*>(&svraddr),
			sizeof(svraddr),
			0,
			0))
		{
			printf_s("WSASendTo Error: %d\n", GetLastError());
			//break;
		}

		printf_s("WSASendTo: pid:%d, count:%d\n", pid, count);


		DWORD recvBytes = 0;
		DWORD cli_peek_flags = 0;// MSG_PEEK;
		char pBufferRecv[RECV_BUFFER_SIZE] = {};
		WSABUF cli_buffer;
		cli_buffer.buf = pBufferRecv;
		cli_buffer.len = RECV_BUFFER_SIZE;
		int addr_size = sizeof(svraddr);
		int res3 = WSARecvFrom(hSock, &cli_buffer, 1, &recvBytes, &cli_peek_flags, (struct sockaddr*)&svraddr, &addr_size, NULL, NULL);
		int pid2 = 0;
		memcpy(&pid2, &pBufferRecv[0], 4);
		int count2 = 0;
		memcpy(&count2, &pBufferRecv[4], 4);
		printf_s("WSARecvFrom: %d, %d\n", pid2, count2);

		Sleep(500); //0.5sec
		count++;

	}

	//뒤처리

	return 0;
}
