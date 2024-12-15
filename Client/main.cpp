#include "Client.hpp"
#include <cstring>

int main() {
	char serverIP[33];
	std::cin.getline(serverIP, 33);

	Client client;
	client.connectToServer(serverIP, "8080");

	client.requestDownloadFile("Files/test.txt");
	client.requestDownloadFile("Files/word.docx");
	client.requestDownloadFile("Files/video.mp4");
}