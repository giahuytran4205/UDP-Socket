#include "Client.hpp"

std::vector<IHandleSignal*> IHandleSignal::m_listeners;

Client::Client() {
	m_socket = INVALID_SOCKET;
	m_seq = 0;
	m_curFileSize = 0;
	m_curFileSize = 0;
	m_running = false;
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

	m_running = true;
	return true;
}

void Client::start() {
	std::signal(SIGINT, ::handleSignal);
	std::thread renderUI([this]() { updateUI(); });

	updateInputFile("input.txt");

	renderUI.join();
}

bool Client::disconnect() {
	int iResult = closesocket(m_socket);
	m_socket = INVALID_SOCKET;
	WSACleanup();
	m_running = false;
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
			
			if (recvLen < 0) {
				std::cerr << "Error with code " << WSAGetLastError();
				continue;
			}

			ack[recvLen] = '\0';
			if (recvLen > 0 && strcmp(ack, (std::string("ACK=") + std::to_string(m_seq)).c_str()) == 0) {
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
		std::string ackMessage = std::string("ACK=") + std::to_string(m_ack).c_str();
		m_ack += bytesReceived;
		sendto(m_socket, ackMessage.c_str(), ackMessage.length(), 0, (sockaddr*)&m_serverAddress, sizeof(m_serverAddress));
	}
	if (bytesReceived == 0) {
		disconnect();
	}

	return bytesReceived;
}

void Client::updateInputFile(const std::string& path) {
	while (m_running) {
		std::vector<std::string> files = readFilePath(path);

		for (std::string& file : files) {
			if (m_files.find(file) == m_files.end()) {
				m_files[file] = FileInfo{ 0, 0 };
				requestDownloadFile(file.c_str());
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	}
}

void Client::requestDownloadFile(const char* path) {
	send(path, strlen(path));

	int bytesReceived = receive((char*)&m_totalFileSize, sizeof(m_totalFileSize));
	m_curFileSize = 0;
	m_files[path].totalFileSize = m_totalFileSize;

	int curFileSize = 0;
	std::fstream fOut(path, std::ios::out | std::ios::binary);

	if (!fOut) {
		std::cerr << "Cannot write to file!";
		return;
	}

	Packet packet;
	int serverAddrLen = sizeof(m_serverAddress);

	std::map<long long, bool> checked;

	while (m_curFileSize < m_totalFileSize) {
		int bytesReceived = recvfrom(m_socket, (char*)&packet, sizeof(packet), 0, (sockaddr*)&m_serverAddress, &serverAddrLen);

		if (bytesReceived < 0) {
			std::cerr << WSAGetLastError();
			return;
		}
		if (bytesReceived > 0) {
			std::string ack = "ACK=";
			ack += std::to_string(packet.info.seq);

			sendto(m_socket, ack.c_str(), ack.size(), 0, (sockaddr*)&m_serverAddress, sizeof(m_serverAddress));

			fOut.seekg(packet.info.offset);
			fOut.write(packet.data, packet.info.length);

			if (!checked[packet.info.seq]) {
				m_curFileSize += packet.info.length;
				checked[packet.info.seq] = true;

				{
					std::lock_guard<std::mutex> lock(m_mutex);
					m_files[path].curFileSize = m_curFileSize;
				}
			}
		}
		if (bytesReceived == 0)
			break;
	}

	fOut.close();
}

long long Client::getFileSize() const {
	return m_totalFileSize;
}

long long Client::getFileProgressInBytes() const {
	return m_curFileSize;
}

float Client::getFileProgessInPercent() const {
	return 1.0 * m_curFileSize / m_totalFileSize;
}

void Client::handleSignal(int signal) {
	if (signal == SIGINT)
		m_running = false;
}

void Client::updateUI() {
	while (m_running) {
		system("cls");
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			for (auto& i : m_files) {
				if (i.second.totalFileSize == 0)
					continue;

				std::string bar(10 * i.second.curFileSize / i.second.totalFileSize, '-');
				bar.append(std::string(10 - bar.size(), ' '));
				bar.append(std::to_string(100.0 * i.second.curFileSize / i.second.totalFileSize) + "%");

				std::cout << i.first << ": " << bar << '\n';
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

void handleSignal(int signal) {
	if (signal == SIGINT) {
		std::cout << "Ctrl + C is pressed\n";
		IHandleSignal::sendSignal(signal);
	}
}