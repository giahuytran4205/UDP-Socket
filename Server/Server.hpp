#pragma once
#include <winsock2.h>
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <mutex>
#include <condition_variable>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

using PORT = const char*;
using IP = const char*;

#define TIMEOUT_MS 2000
#define CHUNK_SIZE 1024

struct PacketInfo {
	long long seq;
	long long offset;
	int length;
	bool operator<(const PacketInfo& p) const {
		return seq < p.seq;
	}
};

struct Packet {
	PacketInfo info;
	char data[CHUNK_SIZE];
};

class Server {
private:
	SOCKET m_socket;
	PORT m_port;
	sockaddr_in m_serverAddress;
	std::mutex m_mutex;
	std::condition_variable m_cv;
	std::string m_curFileTransfer;
	std::set<PacketInfo> m_queue;
	long long m_seq;
	long long m_ack;
	bool m_running;

public:
	Server();
	~Server();

	void startTransfer(sockaddr_in& clientAddr);
	void receiveACK(sockaddr_in& clientAddr);
	void onReceiveFrom(sockaddr_in clientAddr, const char* buf, int length);
	int send(sockaddr_in clientAddr, const char* buf, int length);
	int receive();
	int sendFile(const char* path);
};