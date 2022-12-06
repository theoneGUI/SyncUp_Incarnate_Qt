#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <functional>
#include "Pointer.h"
#include <utility>
using std::pair;
namespace suon {
	const enum class storType {
		ASSOC,
		LIST
	};
	
	const std::string nfound = "&SUON_NF&";
	const std::string objEmpty = "&SUON_EMPTY&";

	class SUON {
	public:
		SUON(suon::storType t) {
			beginList();
			storageMethod = t;
		};
		void add(pair<std::string, std::string>& addTo);
		void dumpToFile(const std::string& fp);
		void loadFromFile(const std::string& fp);
		std::string getJson();
		storType getStorageMethod();
		std::vector<Pointer> getIndexed() { return indexedResults; };
		std::string operator[](std::string );
		auto next();
		bool hasNext() { return !((iteratorIndex + 1) == iteratorMaxIndex); }
		void resetIterator() { iteratorIndex = 0; };
		friend class SUONEval;
#ifdef PRERELEASE
	public:
		std::vector<Pointer> indexedResults;
		suon::storType storageMethod;
		std::string runningString;
		std::vector<pair<std::string, pair<std::string, std::string>>> rapidAccess;
#endif
#ifndef PRERELEASE
	private:
		std::vector<Pointer> indexedResults;
		suon::storType storageMethod;
		std::string runningString;
		std::vector<pair<std::string, pair<std::string, std::string>>> rapidAccess;
#endif
	private:
		void beginList() { runningString = "["; }
		void endList() { if (runningString[runningString.size()-1] != ']') runningString += "]"; }
		void delimit();
		void indexRapidAccess();
		long long unsigned stringToIntHash(std::string&);
		bool optimized = false; // this tells the object whether or not to re-index upon getting the [] operator called
		bool objectVectorIsUpToDate = false;
		void convertToObjects();
		size_t iteratorIndex = 0;
		size_t iteratorMaxIndex;
	};


	class SUONEval
	{
	public:
		SUONEval(SUON newFren) : fren(newFren) {};
		void collisions() {
			unsigned long long last = 0;
			int collisions = 0;
			for (int j = 0; j < fren.indexedResults.size(); j++) {
				auto i = fren.indexedResults.at(j);
				if (i.pointsTo() == last) {
					collisions++;
				}
				last = i.pointsTo();
			}
			std::cout << "This test resulted in " << collisions << " collisions" << std::endl;
		}
	private:
		SUON fren;
	};

	inline void SUON::add(pair<std::string, std::string>& addTo)
	{
		delimit();
		if (storageMethod == storType::LIST) {
			runningString += "{\"" + addTo.first + "\":\"" + addTo.second + "\"}";
		}
		else {
			runningString += "\"" +  addTo.first + "\":{\"" + addTo.first + "\":\"" + addTo.second + "\"}";
		}
		optimized = false;
		objectVectorIsUpToDate = false;
	}

	inline std::string SUON::getJson()
	{
		if (runningString.at(runningString.size() - 1) == ']') {
			;;
		}
		else {
			endList();
		}
		return runningString;
	}

	inline storType SUON::getStorageMethod()
	{
		return storageMethod;
	}

	inline void SUON::delimit() {
		char lastOfStr = runningString.at(runningString.size() - 1);
		if (lastOfStr == '}') {
			runningString += ", ";
		}
		else if (lastOfStr == ']') {
			runningString.at(runningString.size() - 1) = ',';
			runningString += " ";
		}
	}

	inline void SUON::indexRapidAccess()
	{
		long long unsigned hash;
		
#ifdef PRERELEASE
		std::cout << "hashing all entries" << std::endl;
#endif
		for (auto& entry : rapidAccess) {
			std::string toHash = entry.first;
			hash = stringToIntHash(toHash);

			Pointer t(hash, entry);
			indexedResults.push_back(t);
		}
#ifdef PRERELEASE
		std::cout << "indexing..." << std::endl;
#endif
		std::make_heap(indexedResults.begin(), indexedResults.end());
		std::sort_heap(indexedResults.begin(), indexedResults.end());

#ifdef PRERELEASE
		std::cout << "finished indexing" << std::endl;
#endif
		optimized = true;
	}

	inline long long unsigned SUON::stringToIntHash(std::string& in)
	{
		size_t len = in.size();
		long long unsigned hash;
		hash = len;
		for (int i = 0; i < len; i++) {
			hash += ((in.at(i) % ((hash-1) * (in.at(i)+1))) *((hash + 1) / 2) % (hash + len + i));
			hash += (hash - (in.at(i)))/2;
		}
		return hash;
	}

	inline void SUON::convertToObjects()
	{
		resetIterator();
		std::vector<std::string> splitted;
		std::vector<size_t> splits;
		size_t len = runningString.length() - 1;
		for (size_t i = 0; i < len; i++) {
			if (i == 0) {
				splits.push_back(2);
				continue;
			}
			if (runningString[i] == ',' && runningString[i + 1] == ' ') {
				splitted.push_back(runningString.substr(splits.back(), ((i - 1) - splits.back())));
				splits.pop_back();
				splits.push_back(i + 2);
			}
		}
		rapidAccess.clear();
		for (const auto& i : splitted) {
			size_t ilen = i.size();
			std::string copy = i;
			pair<std::string, pair<std::string, std::string>> fillMe;
			std::vector<std::string> partitions;
			bool badbit = false;
			size_t qPos;
			while (copy.size() > 0 && !badbit) {
				qPos = copy.find_first_of('"');
				if (copy[0] != '"' && copy[0] != ':') {
					std::string copyOfCopy = copy.substr(0, (copy.size() - (copy.size() - copy.find_first_of('"'))));
					partitions.push_back(copyOfCopy);
				}
				if (copy[0] == '"')
					copy = copy.substr(1, copy.size() - 1);

				if (qPos == std::string::npos) {
#ifdef PRERELEASE
					std::cout << "File name too long to process" << std::endl;
#endif
					badbit = true;
					break;
				}
				copy = copy.substr(qPos, (i.size() - (qPos + 1)));
			}
			if (badbit == false) {
				fillMe.first = (partitions[0]);
				pair<std::string, std::string> tmp;
				tmp.first = (partitions[1]);
				tmp.second = (partitions[2]);
				fillMe.second = (tmp);
				rapidAccess.push_back(fillMe);
			}

		}
		indexRapidAccess();
		objectVectorIsUpToDate = true;
	}

	inline void SUON::dumpToFile(const std::string& fp)
	{
		endList();
		std::ofstream fileOut(fp);
		if (storageMethod == storType::ASSOC) {
			fileOut << "ASSOC\n";
		}
		else {
			fileOut << "LIST\n";
		}
		fileOut << runningString;

		fileOut.close();
	}

	inline void SUON::loadFromFile(const std::string& fp) {
		std::ifstream fileIn(fp);

		std::string fileInType;

		bool passedTypeDef = false;
		while (fileIn.good()) {
			char thisChar = fileIn.get();
			if (thisChar == '\n') {
				if (fileInType == "ASSOC") {
					storageMethod = storType::ASSOC;
				}
				else if (fileInType == "LIST") {
					storageMethod = storType::LIST;
				}
				passedTypeDef = true;
				continue;
			}

			if (passedTypeDef) {
				runningString += thisChar;
			}
			else {
				fileInType += thisChar;
			}
		}

		convertToObjects();
	}
	
	inline std::string SUON::operator[](std::string hashd)
	{
		if (!objectVectorIsUpToDate)
			convertToObjects();
		else if (!optimized)
			indexRapidAccess();
		else if (indexedResults.size() == 0)
			return objEmpty;
		long long unsigned indexHash = stringToIntHash(hashd);

		size_t indexedLen = indexedResults.size();
		int low = 0;
		int high = indexedLen;


		pair<std::string, pair<std::string, std::string>> result;

		while (low<=high) {
			int middle = (low + high) / 2;
			auto atMiddle = indexedResults[middle].pointsTo();
			if (indexHash == atMiddle) {
				if (indexedResults[middle].is().second.second == hashd)
					return indexedResults[middle].is().second.second;
				int tmp = middle;
				while (middle != 0 && indexedResults[tmp - 1].pointsTo() == indexedResults[tmp].pointsTo()) { // in the event of a collision (backwards)
					if (indexedResults[tmp].is().second.second == hashd)
						return indexedResults[tmp].is().second.second;
					tmp--;
				}
				tmp = middle;
				while (middle != indexedLen-1 && indexedResults[tmp + 1].pointsTo() == indexedResults[tmp].pointsTo()) { // in the event of a collision (forwards)
					if (indexedResults[tmp].is().second.second == hashd)
						return indexedResults[tmp].is().second.second;
					tmp++;
				}
				return indexedResults[middle].is().second.second;
			}
			else if (indexHash < atMiddle) {
				high = middle - 1;
			}
			else {
				low = middle + 1;
			}
		}

		return nfound;
	}

	inline auto SUON::next()
	{
		if (!objectVectorIsUpToDate)
			convertToObjects();
		iteratorMaxIndex = indexedResults.size();
		iteratorIndex++;
		if (indexedResults.size() == 0)
			return pair<std::string, pair<std::string, std::string>>();
		return indexedResults.at(iteratorIndex - 1).is();
	}
	
}



