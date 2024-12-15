#pragma once
#include <winsock2.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

using PORT = const char*;
using IP = const char*;

#define TIMEOUT_MS 2000

struct Packet {
	Packet() = default;
	int offset;
	char data[256];
};

class Server {
private:
	SOCKET m_socket;
	PORT m_port;
	sockaddr_in m_serverAddress;
	long long m_seq;
	long long m_ack;

public:
	Server();
	~Server();

	void onReceiveFrom(sockaddr_in clientAddr, const char* buf, int length);
	int send(sockaddr_in clientAddr, const char* buf, int length);
	int receive();
	int sendFile(const char* path);
};