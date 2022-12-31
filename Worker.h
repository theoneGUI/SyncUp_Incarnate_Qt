#pragma once
#include <QObject>
#include <QThread>
#include <QString>
#include "Hasher.h"
#include <string>
#include <vector>
#include <iostream>
#include "include/sha256.h"
#include <fstream>
#include "include/JSON.h"
#include <boost/filesystem.hpp>
#include <system_error>

#define PRERELEASE

class WorkerThread : public QThread
{
    Q_OBJECT
public:

	WorkerThread(std::string& directoryPath, std::string& hashStorageFile);
	WorkerThread(const char* directoryPath, const char* hashStorageFile);

	WorkerThread(QString directoryPath, QString hashStorageFile);

	std::vector<std::string> run(const bool& dumpToFile = true);
	size_t size() const;
private:
	std::string dirToHash;
	std::string hashFile;
	suon::SUON accumulator;

	std::vector<std::string> privateRun(const bool& dumpToFile);

	void hashSingleFile(const std::string& fp);
	suon::SUON hashDirectory(const std::string& dp);
	int numChanges;


public:
	void run() override {
		privateRun(
#ifdef PRERELEASE
			false
#else
			true
#endif
		);
	}
private:
    QString scan;
    QString store;
signals:
    void resultReady(long long);
	void totalFiles(long long);
	void fileError(int);
	void hashedFileCountIncrement(int);
	void changedFilesReady(std::vector<std::string>);
};

