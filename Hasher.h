#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#pragma once
#ifndef HASHER_H
#define HASHER_H
#include <iostream>
#include "include/sha256.h"
#include <fstream>
#include <string>
#include "include/JSON.h"
#include <QString>
#include <filesystem>
#include <system_error>

class Hasher {
public:
	Hasher(std::string& directoryPath, std::string& hashStorageFile);
	Hasher(const char* directoryPath, const char* hashStorageFile);

	Hasher(QString directoryPath, QString hashStorageFile);

	std::vector<std::string> run(const bool& dumpToFile = true);
	size_t size() const;
private:
	std::string dirToHash;
	std::string hashFile;
	suon::SUON accumulator;

	std::vector<std::string> privateRun(const bool& dumpToFile);

	void hashSingleFile(const std::string& fp);
	suon::SUON hashDirectory(const std::string& dp);
	size_t numChanges;
};




#endif