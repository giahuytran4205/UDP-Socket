#include "Server.hpp"

Server::Server() {
	m_socket = INVALID_SOCKET;
	m_seq = 0;

	WSAData wsaData;
	int iResult;
	struct addrinfo* result = nullptr, * ptr = nullptr, hints;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	m_serverAddress.sin_family = AF_INET;
	m_serverAddress.sin_port = htons(8080);
	m_serverAddress.sin_addr.s_addr = INADDR_ANY;

	if (bind(m_socket, (sockaddr*)&m_serverAddress, sizeof(m_serverAddress)) == SOCKET_ERROR) {
		std::cout << "Bind failed: " << WSAGetLastError() << "\n";
		closesocket(m_socket);
		WSACleanup();
		return;
	}
}

Server::~Server() {

}

void Server::onReceiveFrom(sockaddr_in clientAddr, const char* buf, int length) {
	std::fstream file(buf, std::ios::in | std::ios::binary);

	if (!file) {
		std::cerr << "Cannot open file!";
		return;
	}

	file.seekg(0, std::ios::end);
	int filesize = file.tellg();
	send(clientAddr, (char*)&filesize, sizeof(filesize));

	file.seekg(0, std::ios::beg);

	char buffer[256];
	while (!file.eof()) {
		Packet packet = Packet();

		packet.offset = file.tellg();
		file.read(packet.data, 256);
		int bytesRead = file.gcount();

		send(clientAddr, (char*)&packet, bytesRead + sizeof(packet.offset));
	}

	file.close();
}

int Server::send(sockaddr_in clientAddr, const char* buf, int length) {
	//std::string header = "SEQ=" + std::to_string(m_seq) + ",LEN=" + std::to_string(length);
	const char* data = buf;

	int iResult;
	bool ackReceived = false;
	int attempts = 0;
	const int maxAttempts = 5;

	while (!ackReceived && attempts < maxAttempts) {
		iResult = sendto(m_socket, data, length, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));

		fd_set readSet;
		FD_ZERO(&readSet);
		FD_SET(m_socket, &readSet);

		timeval timeout;
		timeout.tv_sec = TIMEOUT_MS / 1000;
		timeout.tv_usec = (TIMEOUT_MS % 1000) * 1000;

		char ack[30];
		int clientAddrLen = sizeof(clientAddr);
		int result = select(0, &readSet, nullptr, nullptr, &timeout);
		if (result > 0) {
			int recvLen = recvfrom(m_socket, ack, sizeof(ack), 0, (sockaddr*)&clientAddr, &clientAddrLen);
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

int Server::receive() {
	char buffer[256];
	sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);

	while (true) {
		int bytesReceived = recvfrom(m_socket, buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr, &clientAddrLen);
		if (bytesReceived == SOCKET_ERROR) {
			std::cerr << "recvfrom failed: " << WSAGetLastError() << "\n";
			break;
		}

		buffer[bytesReceived] = '\0';
		std::cout << buffer << '\n';
		m_ack += bytesReceived;
		std::string ackMessage = std::string("ACK=") + std::to_string(m_ack).c_str();
		sendto(m_socket, ackMessage.c_str(), ackMessage.size(), 0, (sockaddr*)&clientAddr, clientAddrLen);
		onReceiveFrom(clientAddr, buffer, bytesReceived);
	}
}

int Server::sendFile(const char* path) {
	return 0;
}