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

class Client {
private:
	PORT m_serverPort;
	PORT m_port;
	SOCKET m_socket;
	sockaddr_in m_serverAddress;
	long long m_seq;
	long long m_ack;
	int m_numConnection;

public:
	Client(int numConnection = 4);
	~Client();

	bool connectToServer(IP serverIP, PORT serverPort);
	bool disconnect();
	int send(const char* buf, int length);
	int receive(char* buf, int length);
	//int sendFile(const char* path);
	void requestDownloadFile(const char* path);
};