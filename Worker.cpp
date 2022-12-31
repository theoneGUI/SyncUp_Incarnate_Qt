#include "Worker.h"

using namespace std;

WorkerThread::WorkerThread(string& directoryPath, string& hashStorageFile) : accumulator(suon::SUON(suon::storType::ASSOC)), dirToHash{ directoryPath }, hashFile{ hashStorageFile } {
	numChanges = 0;
	if (!filesystem::exists(filesystem::path(directoryPath)))
		throw bad_exception();
	if (!filesystem::exists(filesystem::path(hashStorageFile)))
		throw exception();
	scan = directoryPath.c_str();
	store = hashStorageFile.c_str();
};

WorkerThread::WorkerThread(const char* directoryPath, const char* hashStorageFile) : accumulator(suon::SUON(suon::storType::ASSOC)), dirToHash{ directoryPath }, hashFile{ hashStorageFile } {
	numChanges = 0;
	if (!filesystem::exists(filesystem::path(directoryPath)))
		throw bad_exception();
	if (!filesystem::exists(filesystem::path(hashStorageFile)))
		throw exception();
	scan = directoryPath;
	store = hashStorageFile;
};

WorkerThread::WorkerThread(QString directoryPath, QString hashStorageFile) : accumulator(suon::SUON(suon::storType::ASSOC)) {
	dirToHash = directoryPath.toStdString();
	hashFile = hashStorageFile.toStdString();
	numChanges = 0;
	if (!filesystem::exists(filesystem::path(dirToHash)))
		throw bad_exception();
	if (!filesystem::exists(filesystem::path(hashFile)))
		throw exception();
	scan = directoryPath;
	store = hashStorageFile;
}

vector<string> WorkerThread::run(const bool& dumpToFile) {
	return privateRun(dumpToFile);
}

size_t WorkerThread::size() const {
	return numChanges;
};

vector<string> WorkerThread::privateRun(const bool& dumpToFile) {
	suon::SUON currentHashes = hashDirectory(dirToHash);
	vector<string> changes;

	ofstream logfile = ofstream("C:/Users/Aidan/Desktop/log.log", std::ios::app);

	try {
		suon::SUON test(suon::storType::ASSOC);
		test.loadFromFile(hashFile);

		for (pair<string, pair<string, string>> i; currentHashes.hasNext(); i = currentHashes.next()) {
			logfile << i.first << "\t" << i.second.second << "\n";
			string hashAgainst = test[i.first];
			if (i.second.second != hashAgainst) {
				if (i.first == "")
					continue;
				changes.push_back(i.first);
			}
		}
		/*for (pair<string, pair<string, string>> i; test.hasNext(); i = test.next()) {
			string hashAgainst = currentHashes[i.first];
			if (hashAgainst == suon::nfound) {
				cout << i.first << " was deleted" << endl;
			}
		}*/

		numChanges = changes.size();
#
		if (dumpToFile)
			currentHashes.dumpToFile(hashFile);
	}
	catch (...) {
		if (dumpToFile)
			currentHashes.dumpToFile(hashFile);
		return vector<string>({ "GIVEN HASH FILE NOT FOUND" });
	}

	emit changedFilesReady(changes);

	return changes;
}

suon::SUON WorkerThread::hashDirectory(const string& dp) {
	size_t hashedFiles = 0;
	filesystem::recursive_directory_iterator iterator(dp);
	int noaccess = 0;

	long long ct = 0;
	filesystem::recursive_directory_iterator traversalForCount(dp);
	while (true) {
		try {
			for (const auto& entry : traversalForCount) {
				if (!filesystem::is_directory(entry)) {
					ct++;
				}
			}
			break;
		}
		catch (filesystem::filesystem_error ex) {
			auto code = ex.code();
			iterator.increment(code);
		}
		catch (system_error ex) {
			auto code = ex.code();
			iterator.increment(code);
		}
	}
	emit totalFiles(ct);

	while (true) {
		try {
			for (const auto& entry : iterator) {
				emit hashedFileCountIncrement(hashedFiles);
				if (!filesystem::is_directory(entry)) {
					hashSingleFile(entry.path().u8string());
					hashedFiles++;
				}
			}
			break;
		}
		catch (filesystem::filesystem_error ex) {
			noaccess++;
			auto code = ex.code();
			iterator.increment(code);
			emit fileError(noaccess);
		}
		catch (system_error ex) {
			auto code = ex.code();
			iterator.increment(code);
		}
	}
	return accumulator;
}

void WorkerThread::hashSingleFile(const string& fp) {
	ifstream reader(fp);
	SHA256 hasher;
	char content;
	while (reader.good()) {
		content = reader.get();
		hasher.add(&content, sizeof(content));
	}
	string hash = hasher.getHash();

	reader.close();

	pair<string, string> hashAppend(fp, hash);

	accumulator.add(hashAppend);
}

