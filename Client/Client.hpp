#pragma once
#include <winsock2.h>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <csignal>
#include <atomic>
#include <mutex>
#include "Filehandle.hpp"

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

struct FileInfo {
	long long curFileSize;
	long long totalFileSize;
};

class IHandleSignal {
private:
	static std::vector<IHandleSignal*> m_listeners;
public:
	IHandleSignal() {
		m_listeners.push_back(this);
	}

	static void sendSignal(int signal) {
		for (auto& listener : m_listeners)
			listener->handleSignal(signal);
	}

	virtual void handleSignal(int signal) = 0;
};

class Client : public IHandleSignal {
private:
	PORT m_serverPort;
	PORT m_port;
	SOCKET m_socket;
	sockaddr_in m_serverAddress;
	std::map<std::string, FileInfo> m_files;
	std::mutex m_mutex;
	long long m_curFileSize;
	long long m_totalFileSize;
	long long m_seq;
	long long m_ack;
	bool m_running;

public:
	Client();
	~Client();

	bool connectToServer(IP serverIP, PORT serverPort);
	void start();
	bool disconnect();
	int send(const char* buf, int length);
	int receive(char* buf, int length);
	void updateInputFile(const std::string& path);
	void requestDownloadFile(const char* path);
	long long getFileSize() const;
	long long getFileProgressInBytes() const;
	float getFileProgessInPercent() const;
	void handleSignal(int signal) override;
	void updateUI();
};

void handleSignal(int signal);