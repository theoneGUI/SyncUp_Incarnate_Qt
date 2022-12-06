#include "Hasher.h"
using namespace std;

Hasher::Hasher(string& directoryPath, string& hashStorageFile) : accumulator(suon::SUON(suon::storType::ASSOC)), dirToHash{ directoryPath }, hashFile{ hashStorageFile } {
	numChanges = 0;
	if (!filesystem::exists(filesystem::path(directoryPath)))
		throw bad_exception();
	if (!filesystem::exists(filesystem::path(hashStorageFile)))
		throw exception();
};

Hasher::Hasher(const char* directoryPath, const char* hashStorageFile) : accumulator(suon::SUON(suon::storType::ASSOC)), dirToHash{ directoryPath }, hashFile{ hashStorageFile } {
	numChanges = 0;
	if (!filesystem::exists(filesystem::path(directoryPath)))
		throw bad_exception();
	if (!filesystem::exists(filesystem::path(hashStorageFile)))
		throw exception();
};

Hasher::Hasher(QString directoryPath, QString hashStorageFile) : accumulator(suon::SUON(suon::storType::ASSOC)) {
	dirToHash = directoryPath.toStdString();
	hashFile = hashStorageFile.toStdString();
	numChanges = 0;
	if (!filesystem::exists(filesystem::path(dirToHash)))
		throw bad_exception();
	if (!filesystem::exists(filesystem::path(hashFile)))
		throw exception();
}

vector<string> Hasher::run(const bool& dumpToFile) {
	return privateRun(dumpToFile);
}

size_t Hasher::size() const { 
	return numChanges;
};

vector<string> Hasher::privateRun(const bool& dumpToFile) {
	suon::SUON currentHashes = hashDirectory(dirToHash);
	vector<string> changes;

	try {
		suon::SUON test(suon::storType::LIST);
		test.loadFromFile(hashFile);
		for (pair<string, pair<string, string>> i; currentHashes.hasNext(); i = currentHashes.next()) {
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

		numChanges = changes.size() - 10;
		if (dumpToFile)
			currentHashes.dumpToFile(hashFile);
	}
	catch (...) {
		if (dumpToFile)
			currentHashes.dumpToFile(hashFile);
		return vector<string>({ "GIVEN HASH FILE NOT FOUND" });
	}


	return changes;
}

suon::SUON Hasher::hashDirectory(const string& dp) {
	size_t hashedFiles = 0;
	filesystem::recursive_directory_iterator iterator(dp);
	int noaccess = 0;
	while (true) {
		try {
			for (const auto& entry : iterator) {
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
		}
		catch (system_error ex) {
			auto code = ex.code();
			iterator.increment(code);
		}
	}
	cout << "Hashed a total of " << hashedFiles << " files" << endl;
	return accumulator;
}

void Hasher::hashSingleFile(const string& fp) {
	ifstream reader(fp);
	SHA256 hasher;
	while (reader.good()) {
		char content = reader.get();
		hasher.add(&content, sizeof(content));
	}
	string hash = hasher.getHash();

	reader.close();

	pair<string, string> hashAppend(fp, hash);

	accumulator.add(hashAppend);
}

