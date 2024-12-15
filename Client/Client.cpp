#include "Client.hpp"

Client::Client(int numConnection) {
	m_numConnection = numConnection;
	m_socket = INVALID_SOCKET;
	m_seq = 0;
}

Client::~Client() {

}

bool Client::connectToServer(IP serverIP, PORT serverPort) {
	WSAData wsaData;
	int iResult;
	struct addrinfo* result = nullptr, * ptr = nullptr, hints;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return false;
	}

	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_socket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return false;
	}

	m_serverAddress.sin_family = AF_INET;
	m_serverAddress.sin_port = htons(8080);
	inet_pton(AF_INET, serverIP, &m_serverAddress.sin_addr);
	return true;
}

bool Client::disconnect() {
	int iResult = closesocket(m_socket);
	m_socket = INVALID_SOCKET;
	WSACleanup();
	return iResult;
}

int Client::send(const char* buf, int length) {
	// Add header
	std::string header;
	const char* data = (header.append(buf)).c_str();
	
	int iResult;
	bool ackReceived = false;
	int attempts = 0;
	const int maxAttempts = 5;

	while (!ackReceived && attempts < maxAttempts) {
		
		iResult = sendto(m_socket, data, length, 0, (sockaddr*)&m_serverAddress, sizeof(m_serverAddress));

		fd_set readSet;
		FD_ZERO(&readSet);
		FD_SET(m_socket, &readSet);

		timeval timeout;
		timeout.tv_sec = TIMEOUT_MS / 1000;
		timeout.tv_usec = (TIMEOUT_MS % 1000) * 1000;

		char ack[30];
		int serverAddrLen = sizeof(m_serverAddress);
		int result = select(m_socket + 1, &readSet, nullptr, nullptr, &timeout);
		if (result > 0) {
			int recvLen = recvfrom(m_socket, ack, sizeof(ack), 0, (sockaddr*)&m_serverAddress, &serverAddrLen);
			ack[recvLen] = '\0';
			if (recvLen > 0 && strcmp(ack, (std::string("ACK=") + std::to_string(m_seq + length)).c_str()) == 0) {
				ackReceived = true;
				m_seq += length;
			}
		}
		else if (result == 0) {
			attempts++;
		}
		else {
			break;
		}
	}

	return iResult;
}

int Client::receive(char* buf, int length) {
	int addrLength = sizeof(m_serverAddress);
	int bytesReceived = recvfrom(m_socket, buf, length, 0, (sockaddr*)&m_serverAddress, &addrLength);
	if (bytesReceived > 0) {
		m_ack += bytesReceived;
		std::string ackMessage = std::string("ACK=") + std::to_string(m_ack).c_str();
		sendto(m_socket, ackMessage.c_str(), ackMessage.length(), 0, (sockaddr*)&m_serverAddress, sizeof(m_serverAddress));
	}
	if (bytesReceived == 0) {
		disconnect();
	}

	return bytesReceived;
}

void Client::requestDownloadFile(const char* path) {
	send(path, strlen(path));

	int filesize = 0;
	int bytesReceived = receive((char*)&filesize, sizeof(filesize));

	int curFileSize = 0;
	std::fstream fOut(path, std::ios::out | std::ios::binary);

	if (!fOut) {
		std::cerr << "Cannot write to file!";
		return;
	}

	Packet packet;
	while (curFileSize < filesize) {
		packet.offset = 0;
		int byteReceived = receive((char*)&packet, sizeof(packet));		
		std::cout << byteReceived << " " << curFileSize << '\n';
		if (byteReceived == 0)
			break;

		fOut.seekg(packet.offset);
		fOut.write(packet.data, byteReceived - sizeof(packet.offset));

		curFileSize += byteReceived - 4;
	}

	fOut.close();
}