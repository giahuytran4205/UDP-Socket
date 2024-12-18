#include "Filehandle.hpp"

std::vector<std::string> readFilePath(const std::string& path) {
	std::fstream fIn(path, std::ios::in);

	if (!fIn) {
		std::cerr << "Cannot open file!";
		return {};
	}

	std::vector<std::string> res;
	std::string filename;
	while (std::getline(fIn, filename)) {
		res.push_back(filename);
	}

	return res;
}