#pragma once
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING

#ifndef MATCHMAKER_H
#define MATCHMAKER_H
#define PRERELEASE

#ifdef PRERELEASE
#define MM_ADDRESS "localhost"
#define MM_PORT 8080
#endif
#ifndef PRERELEASE
#define MM_ADDRESS "syncup.thatonetechcrew.net"
#define MM_PORT 8080
#endif

#define MM_SEND 0
#define MM_RECV 1
#define MM_IS_REMOTE 1
#define MM_IS_LOCAL 0
#define MM_IS_UNDEFINED -1
typedef int mm_role;

#include <stdio.h>
#include <stdlib.h>
#include <plibsys.h>
#include <psocket.h>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <regex>
#include "../../../../commons/Traversal.h"
#include <algorithm>
#include <boost/filesystem.hpp>
#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include "../../commons/SUFTP.h"
#include "DNSResolv.h"
#include "../../curl-7.70.0/include/curl/curl.h"
#include <utility>
#include <QThread>
#include <QObject>
#include "../../../../commons/commons.h"
#include <chrono>
#include <ctime>

#define MAX_MESSAGE_LENGTH 8192
#define SERVER_PORT 4444

using std::endl;

std::string exec(const char* cmd);

std::vector<std::string> getAllIPs();

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

std::string getPublicIP();

std::vector<std::string> sepString(const std::string& str, const std::string& beginning, const std::string& end, bool truncEnd = true, bool truncBegin = true);

std::string singleSearch(const std::string& str, const std::string& beginning, const std::string& end, bool truncEnd = true, bool truncBegin = true);

struct address {
	std::string ipv4;
	int exit_code;
	int addrType = MM_IS_UNDEFINED;
};

class Matchmaker : public QThread, public suftp::plib_user
{
	Q_OBJECT
public:
	Matchmaker(mm_role role) {
		isRunning = false;
		this->role = role;
	}

	void run() override {
		logfile = std::ofstream(paths::SYNCUP_DATA_DIR + "mm.log", std::ios::app);
		auto now = std::chrono::system_clock::now();
		auto formatted = std::chrono::system_clock::to_time_t(now);
		logfile << "\n\nNew log beginning - " << std::ctime(&formatted) << endl;
		address failaddr = address{ "failed", 1 };
		if (isRunning)
			emit matchFailed(19);
		isRunning = true;
		address code = privateRun();
		isRunning = false;
		logfile << "Match made: \n" << "IPv4: " << code.ipv4 << "\nAddress type: " << (code.addrType == MM_IS_REMOTE ? "REMOTE" : "LOCAL") << endl;
		logfile.close();
		emit matchMade(code);
	}
private:
	int role;
	bool isRunning;
	address privateRun() {
		PSocketAddress* addr;
		PSocket* sock;

		// Construct address for server.  Since the server is assumed to be on the same machine for the sake of this program, the address is loopback, but typically this would be an external address.
		Resolver resolver;
		auto dns = resolver.resolve(MM_ADDRESS);
		size_t index = dns.size() - 1;
		auto resolved = dns[index].c_str();
		bool failed = false;
		if ((addr = p_socket_address_new(resolved, MM_PORT)) == NULL)
			emit matchFailed(1);

		// Create socket
		if ((sock = p_socket_new(P_SOCKET_FAMILY_INET, P_SOCKET_TYPE_STREAM, P_SOCKET_PROTOCOL_TCP, NULL)) == NULL)
		{
			// Can't construct socket, cleanup and exit.
			p_socket_address_free(addr);

			emit matchFailed(2);
		}
		// Connect to server.
		if (!p_socket_connect(sock, addr, NULL))
		{
			// Couldn't connect, cleanup.
			p_socket_address_free(addr);
			p_socket_free(sock);
			emit matchFailed(4);
		}
		
		if (failed)
		{
			logfile << "Failed to connect to matchmaking service\n";
			return address{ "failed", 1 };
		}
		logfile << "Connected to matchmaking service successfully" << endl;

		std::string sendHeaders = suftp::headStrtSig;
		char garbage[4]{};
		std::vector<std::pair<std::string, std::string>> otherHeaders;
		otherHeaders.push_back(std::pair<std::string, std::string>("USERID", "PREALPHA_stuff"));
		otherHeaders.push_back(std::pair<std::string, std::string>("PUBLICIP ", getPublicIP().c_str()));
		otherHeaders.push_back(std::pair<std::string, std::string>("ROLE", (role == MM_RECV ? "RECV" : "SEND")));
		auto ips = getAllIPs();
		std::string ipv4s;
		for (int i = 0; i < ips.size(); i++) {
			ipv4s += ips[i];
			if (i != ips.size() - 1)
				ipv4s += ",";
		}
		otherHeaders.push_back(std::pair<std::string, std::string>("LOCALIP", ipv4s));

		for (auto& otherHeader : otherHeaders) {
			sendHeaders += createHeader(otherHeader.first, otherHeader.second);
		}

		char* headerBuffer = new char[MAX_MESSAGE_LENGTH + 2];
		for (int i = 0; i < MAX_MESSAGE_LENGTH; i++) {
			headerBuffer[i] = 0;
		}

		// send headers
		for (int i = 0; ; i++) {
			if (i % MAX_MESSAGE_LENGTH == 0) {
				p_socket_send(sock, headerBuffer, MAX_MESSAGE_LENGTH, NULL);
			}
			else if (i == sendHeaders.size() - 1) {
				p_socket_send(sock, headerBuffer, i % MAX_MESSAGE_LENGTH, NULL);
				break;
			}
			headerBuffer[i % MAX_MESSAGE_LENGTH] = sendHeaders[i];
		}

		p_socket_send(sock, suftp::headTermSig.c_str(), sizeof(suftp::headTermSig) + 1, NULL);
		char* raw = new char[(MAX_MESSAGE_LENGTH + 2)];
		std::string receivedHeaders;


		for (int i = 0; i < MAX_MESSAGE_LENGTH; i++) {
			headerBuffer[i] = 0;
		}


		auto q = waitAndReceiveHeaders(sock);
		std::vector<Traversal> filesToReceive;
		std::vector<Traversal> otherParsedHeaders;
		for (auto& i : q) {
			if (i.first == "FILE") {
				std::vector<std::pair<std::string, std::string>> w = i.second;
				Traversal r(w);
				filesToReceive.push_back(r);
			}
			else {
				std::vector<std::pair<std::string, std::string>> w = i.second;
				Traversal r(w);
				otherParsedHeaders.push_back(r);
			}
		}

		bool rejected = false;
		for (auto& i : otherParsedHeaders) {
			if (i["STATUS"].find("REJECTED") != std::string::npos) {
				rejected = true;
				logfile << "Rejected" << endl;
				break;
			}
		}

		std::string addrToUse;
		int useRemote = MM_IS_UNDEFINED;
		if (!rejected) {
			// Signal the sender that we're ready
			p_socket_send(sock, "R", 1, NULL);
			auto received = waitAndReceiveHeaders(sock);
			for (auto& i : received) {
				for (auto& j : i.second) {
					if (j.first == "TARGETADDR")
						addrToUse = j.second;
					if (j.first == "REMOTE")
						useRemote = (j.second == "True" ? MM_IS_REMOTE : MM_IS_LOCAL);
					logfile << j.first  << "->" << j.second << endl;
				}
			}
		}

		delete[] headerBuffer;
		delete[] raw;
		p_socket_address_free(addr);
		p_socket_close(sock, NULL);
		logfile << "Match made successfully, contintuing onto SUFTP" << endl;
		return address{ addrToUse, 0, useRemote };
	}
	std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> parseHeaders(std::string headers) {
		std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> parsedHeaders;
		auto fileBlock = singleSearch(headers, suftp::headFileStrt, suftp::headerBlockSeparator, false, false);
		auto otherBlock = singleSearch(headers, suftp::headerBlockSeparator, suftp::headTermSig, false, true);
		auto headsInOtherBlock = sepString(otherBlock, suftp::headerTitleStrt, "\n", false, false);
		if (fileBlock == "NOT_FOUND" || otherBlock.size() == 0)
			return parsedHeaders; // invalid header - a header requires both a file block AND an 'other' block for SUFTP

		auto filesInHeaders = sepString(fileBlock, suftp::headFileStrt, suftp::headFileTerm);

		for (auto& file : filesInHeaders) {
			std::pair<std::string, std::vector<std::pair<std::string, std::string>>> thisFile;
			std::vector<std::pair<std::string, std::string>> theseHeaders;
			file += "\n";
			auto argumentsForFile = sepString(file, suftp::headerTitleStrt, "\n", false, false);

			for (auto& arg : argumentsForFile) {
				auto title = singleSearch(arg, suftp::headerTitleStrt, suftp::headAssociativeOperator);
				auto value = singleSearch(arg, suftp::headAssociativeOperator, "\n");
				if (title == "FPATH")
					thisFile.first=("FILE");
				theseHeaders.push_back(std::pair<std::string, std::string>(title, value));
			}
			thisFile.second=(theseHeaders);
			parsedHeaders.push_back(thisFile);
		}

		for (auto& other : headsInOtherBlock) {
			std::string headerName = singleSearch(other, suftp::headerTitleStrt, suftp::headAssociativeOperator);
			std::string headerValue = singleSearch(other, suftp::headAssociativeOperator, "\n");
			auto head = std::pair<std::string, std::vector< std::pair<std::string, std::string>>>();
			auto arg = std::vector<std::pair<std::string, std::string>>();
			arg.push_back(std::pair<std::string, std::string>(headerName, headerValue));
			head.first=(headerName);
			head.second=(arg);
			parsedHeaders.push_back(head);
		}
		return parsedHeaders;
	}
	std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> waitAndReceiveHeaders(PSocket* sock) {
		char* headerBuffer = new char[MAX_MESSAGE_LENGTH + 2];
		for (int i = 0; i < MAX_MESSAGE_LENGTH + 2; i++) {
			headerBuffer[i] = 0;
		}
		std::string receivedHeaders = "";
		while (true) {
			pssize sizeOfRecvData = p_socket_receive(sock, headerBuffer, MAX_MESSAGE_LENGTH, NULL);
			headerBuffer[MAX_MESSAGE_LENGTH + 1] = '\0';
			receivedHeaders += headerBuffer;
			if (receivedHeaders.find(suftp::headTermSig) != std::string::npos)
				break;
		}
		auto responseFromServer = parseHeaders(receivedHeaders);
		delete[] headerBuffer;
		return responseFromServer;
	}
	static std::string createHeader(const std::string& title, const std::string& value) {
		std::string header = suftp::headerTitleStrt;

		for (int i = 0; i < title.size(); i++) {
			char upper = toupper(title[i]);
			if (strchr(suftp::alpha, upper))
				header += toupper(title[i]);
		}

		header += suftp::headAssociativeOperator + value + "\n";
		return header;
	}
	std::ofstream logfile;

signals:
	void matchMade(address);
	void matchFailed(int);

};

#endif