#include "Client.hpp"
#include <cstring>
#include "TUI.hpp"

int main() {
	char serverIP[33];
	//std::cin.getline(serverIP, 33);

	Client client;
	client.connectToServer("192.168.1.161", "8080");

	auto start = std::chrono::high_resolution_clock::now();

	client.start();

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = end - start;

	std::cout << "Total times to download all files: " << duration.count() << '\n';
}