#pragma once
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <stdio.h>
#include <stdlib.h>
#include <plibsys.h>
#include <psocket.h>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <regex>
#include "Traversal.h"
#include <algorithm>
#include <boost/filesystem.hpp>
#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <utility>
#include <QThread>

#define MAX_MESSAGE_LENGTH 8192
#define SERVER_PORT 4444

using std::cout;
using std::pair;
using std::endl;

namespace suftp {

	const char alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	const std::string fileTermSig = "<%SU+TERM+FILE%>";
	const std::string fileStrtSig = "<%SU+STRT+FILE%>";

	const std::string headStrtSig = "<%SU+STRT+HEAD%>";
	const std::string headTermSig = "<%SU+TERM+HEAD%>";

	const std::string streamTermSig = "<%SU+TERM+STREAM%>";
	const std::string streamStrtSig = "<%SU+STRT+STREAM%>";

	const std::string headFileStrt = "%FILE\n";
	const std::string headFileTerm = "%ENDFILE\n";

	const std::string headAssociativeOperator{ ":" };
	const std::string headValArgAssociativeOp{ "=" };
	const std::string headerTitleStrt{ "&" };
	const std::string headerBlockSeparator{ "\n\n" };

	std::string exec(const char* cmd);

	std::vector<std::string> getAllIPs();

	std::vector<std::string> sepString(const std::string& str, const std::string& beginning, const std::string& end, bool truncEnd = true, bool truncBegin = true);

	std::string singleSearch(const std::string& str, const std::string& beginning, const std::string& end, bool truncEnd = true, bool truncBegin = true);

	class SUFTPSender : public QThread
	{
		Q_OBJECT

	public:
		SUFTPSender(std::string interfaceToBindTo, int portToBindTo, std::string rootDirectory, std::vector<std::string> filenames) {
			this->interfaceToBindTo = interfaceToBindTo;
			this->portToBindTo = portToBindTo;
			this->rootDirectory = rootDirectory;
			this->filenames = filenames;
			p_libsys_init();
			isRunning = false;
		}
		~SUFTPSender() {
			p_libsys_shutdown();
		}
		void run() override {
			if (isRunning)
				return;
			isRunning = true;
			int code = privateRun(this->interfaceToBindTo, this->portToBindTo, this->rootDirectory, this->filenames);
			isRunning = false;
		}

	protected:
		bool isRunning;
		int privateRun(std::string interfaceToBindTo, int portToBindTo, std::string rootDirectory, std::vector<std::string> filenames) {
			PSocketAddress* addr;
			PSocket* sock;

			// Binding socket to local host (we are a server, the appropriate port.  Typically this will always be a localhost port, because we are going to listen to this)
			if ((addr = p_socket_address_new(interfaceToBindTo.c_str(), portToBindTo)) == NULL)
				emit transferFailed(1);
			// Create socket
			if ((sock = p_socket_new(P_SOCKET_FAMILY_INET, P_SOCKET_TYPE_STREAM, P_SOCKET_PROTOCOL_TCP, NULL)) == NULL)
			{
				// Failed to create socket -- cleanup
				p_socket_address_free(addr);

				emit transferFailed(2);
			}

			// Bind to local host (server) socket
			if (!p_socket_bind(sock, addr, FALSE, NULL))
			{
				// Couldn't bind socket, cleanup
				p_socket_address_free(addr);
				p_socket_free(sock);

				emit transferFailed(3);
			}

			// Listen for incoming connections on localhost SERVER_PORT
			if (!p_socket_listen(sock, NULL))
			{
				// Couldn't start listening, cleanup
				p_socket_address_free(addr);
				p_socket_close(sock, NULL);

				emit transferFailed(4);
			}

			// Forever, try to accept incoming connections.
			char garbage[4];
			char* buffer = new char[MAX_MESSAGE_LENGTH + 1]; // Supports messages up to max length (plus null character that terminates the std::string, since we're sending text)

			// Blocks until connection accept happens by default -- this can be changed
			PSocket* con = p_socket_accept(sock, NULL);
			std::ifstream file;
			std::string headers;

			for (auto& fname : filenames) {
				size_t invalid = -1;

				file.open(fname, std::ios::binary | std::ios::ate);
				size_t filesize = file.tellg();
				file.close();
				if (filesize == invalid)
					continue;
				headers += headFileStrt;
				std::string relativePath = fname;
				size_t i = fname.find(rootDirectory);
				if (i != std::string::npos) {
					relativePath.replace(i, rootDirectory.size(), "");
					headers += createHeader("FPATH", relativePath);
					headers += createHeader("FPATHTYPE", "REL");
				}
				else {
					headers += createHeader("FPATH", relativePath);
					headers += createHeader("FPATHTYPE", "ABS");
				}
				headers += createHeader("FSIZE", std::to_string(filesize));
				headers += headFileTerm;
			}

			headers += headerBlockSeparator;

			std::vector<pair<std::string, std::string>> otherHeaders;
			otherHeaders.push_back(pair<std::string, std::string>("USERID", "PREALPHA_stuff"));
			for (auto& otherHeader : otherHeaders) {
				headers += createHeader(otherHeader.first, otherHeader.second);
			}

			char* headerBuffer = new char[MAX_MESSAGE_LENGTH + 1];
			for (int i = 0; i < MAX_MESSAGE_LENGTH; i++) {
				headerBuffer[i] = 0;
			}

			// send headers
			for (int i = 0; ; i++) {
				if (i % MAX_MESSAGE_LENGTH == 0) {
					p_socket_send(con, headerBuffer, MAX_MESSAGE_LENGTH, NULL);
					p_socket_receive(con, garbage, sizeof(garbage), NULL);
				}
				else if (i == headers.size() - 1) {
					p_socket_send(con, headerBuffer, i % MAX_MESSAGE_LENGTH, NULL);
					p_socket_receive(con, garbage, sizeof(garbage), NULL);
					break;
				}
				headerBuffer[i % MAX_MESSAGE_LENGTH] = headers[i];
			}
			p_socket_send(con, headTermSig.c_str(), sizeof(headTermSig) + 1, NULL);
			p_socket_receive(con, garbage, sizeof(garbage), NULL);

			// block until recipient is ready for the files themselves
			p_socket_receive(con, garbage, sizeof(garbage), NULL);

			int numFiles = 0;
			for (auto& filename : filenames) {
				if (filename == "")
					continue;
				file.open(filename, std::ios::binary);
				while (!file.eof()) {
					int i = 0;
					for (i = 0; (file.good() && i < MAX_MESSAGE_LENGTH); i++) {
						buffer[i] = file.get();
						if (i != MAX_MESSAGE_LENGTH - 1)
							buffer[i + 1] = 0;
					}

					if (con != NULL)
					{
						p_socket_send(con, buffer, MAX_MESSAGE_LENGTH, NULL);
					}
					else
						printf("Can't make con, tried and failed...\n");
				}
				file.close();
				numFiles += 1;
				emit incrementFilesTransferred(numFiles);
			}

			p_socket_send(con, streamTermSig.c_str(), sizeof(streamTermSig.c_str()) + 1, NULL);
			p_socket_close(con, NULL);

			delete[] headerBuffer;
			delete[] buffer;
			// Cleanup
			p_socket_address_free(addr);
			p_socket_close(sock, NULL);

			emit transferDone();
			return 0;
		}
		static std::string createHeader(const std::string& title, const std::string& value) {
			std::string header = headerTitleStrt;

			for (int i = 0; i < title.size(); i++) {
				char upper = toupper(title[i]);
				if (strchr(alpha, upper))
					header += toupper(title[i]);
			}

			header += headAssociativeOperator + value + "\n";
			return header;
		}

		std::string interfaceToBindTo;
		int portToBindTo;
		std::string rootDirectory;
		std::vector<std::string> filenames;

	public slots:
		void incrementFilesTransferred(int f);
		void transferDone();
		void transferFailed(int code);
	};

	class SUFTPReceiver : public QThread
	{
		Q_OBJECT
	public:
		SUFTPReceiver(std::string interfaceToBindTo, int portToBindTo, std::string rootDirectory) {
			this->interfaceToBindTo = interfaceToBindTo;
			this->portToBindTo = portToBindTo;
			this->rootDirectory = rootDirectory;
			p_libsys_init();
			isRunning = false;
		}
		~SUFTPReceiver() {
			p_libsys_shutdown();
		}

		void run() override {
			if (isRunning)
				return;
			isRunning = true;
			int code = privateRun(interfaceToBindTo, portToBindTo, rootDirectory);
			isRunning = false;
		}
	protected:
		bool isRunning;
		int privateRun(std::string interfaceToBindTo, int portToBindTo, std::string rootDirectory) {
			PSocketAddress* addr;
			PSocket* sock;

			// Construct address for server.  Since the server is assumed to be on the same machine for the sake of this program, the address is loopback, but typically this would be an external address.
			if ((addr = p_socket_address_new(interfaceToBindTo.c_str(), portToBindTo)) == NULL)
				emit transferFailed(1);

			// Create socket
			if ((sock = p_socket_new(P_SOCKET_FAMILY_INET, P_SOCKET_TYPE_STREAM, P_SOCKET_PROTOCOL_TCP, NULL)) == NULL)
			{
				// Can't construct socket, cleanup and exit.
				p_socket_address_free(addr);

				emit transferFailed(2);
			}
			// Connect to server.
			if (!p_socket_connect(sock, addr, NULL))
			{
				// Couldn't connect, cleanup.
				p_socket_address_free(addr);
				p_socket_free(sock);
				emit transferFailed(4);
			}

			char* raw = new char[(MAX_MESSAGE_LENGTH + 2)];
			std::string headers;


			char* headerBuffer = new char[MAX_MESSAGE_LENGTH + 2];
			for (int i = 0; i < MAX_MESSAGE_LENGTH; i++) {
				headerBuffer[i] = 0;
			}

			while (true) {
				pssize sizeOfRecvData = p_socket_receive(sock, headerBuffer, MAX_MESSAGE_LENGTH, NULL);
				headerBuffer[MAX_MESSAGE_LENGTH + 1] = '\0';
				p_socket_send(sock, "R", 1, NULL);
				if (strstr(headerBuffer, headTermSig.c_str()) != nullptr)
					break;
				headers += headerBuffer;

			}

			auto q = parseHeaders(headers);
			std::cout << headers << std::endl;
			std::vector<Traversal> filesToReceive;
			std::vector<Traversal> otherParsedHeaders;
			for (auto& i : q) {
				if (i.first == "FILE") {
					std::vector<pair<std::string, std::string>> w = i.second;
					Traversal r(w);
					filesToReceive.push_back(r);
				}
				else {
					std::vector<pair<std::string, std::string>> w = i.second;
					Traversal r(w);
					otherParsedHeaders.push_back(r);
				}
			}

			bool streamDone = false;

			// Signal the sender that we're ready for all files to be sent in one blob
			p_socket_send(sock, "R", 1, NULL);

			size_t totalContentPacketsExpected = 0;
			for (auto& i : filesToReceive) {
				int expectedPackets = stoi(i["FSIZE"]);
				expectedPackets = (expectedPackets / MAX_MESSAGE_LENGTH);
				totalContentPacketsExpected += expectedPackets;
			}

			size_t totalPacketsReceived = 0;
			size_t f = 0;
			for (auto& currentFileInfo : filesToReceive) {
				std::cout << "Receiving " << currentFileInfo["FPATH"] << std::endl;
				std::string filename = rootDirectory + currentFileInfo["FPATH"];

				int expectedPackets = stoi(currentFileInfo["FSIZE"]);
				int expectedFinalPacketLength = expectedPackets % MAX_MESSAGE_LENGTH;
				expectedPackets = (expectedPackets / MAX_MESSAGE_LENGTH);
				/* In this next block, we're making sure that the parent path for the given file exists.
				If not, make it. */
				boost::filesystem::path fileAsPath(filename);
				if (!boost::filesystem::exists(fileAsPath.parent_path())) {
					try {
						boost::filesystem::create_directories(fileAsPath.parent_path());
					}
					catch (boost::filesystem::filesystem_error e) {
						cout << e.code() << endl;
						cout << e.path1() << endl;
						cout << e.what() << endl;
					}
				}


				std::ofstream fileOut;
				fileOut.open(filename, std::ios::binary);

				for (int packets = 0; ; packets++) {
					pssize sizeOfRecvData = p_socket_receive(sock, raw, MAX_MESSAGE_LENGTH, NULL);
					totalPacketsReceived++;
					raw[sizeOfRecvData] = '\0'; // Set null character 1 after message
					if (totalPacketsReceived == totalContentPacketsExpected + 1) {
						streamDone = true;
						break;
					}

					if (packets == expectedPackets) {
						for (int bits = 0; bits < expectedFinalPacketLength; bits++)
							fileOut << raw[bits];
						break;
					}
					else
						fileOut.write(raw, MAX_MESSAGE_LENGTH);

				}
				fileOut.close();
				f++;
				emit incrementFilesTransferred(f);
				if (streamDone)
					break;
			}
			emit transferDone();
			delete[] headerBuffer;
			delete[] raw;
			p_socket_address_free(addr);
			p_socket_close(sock, NULL);
			return 0;
		}
		static std::vector<pair<std::string, std::vector<pair<std::string, std::string>>>> parseHeaders(std::string headers) {
			std::vector<pair<std::string, std::vector<pair<std::string, std::string>>>> parsedHeaders;
			auto fileBlock = singleSearch(headers, headFileStrt, headerBlockSeparator, false, false);
			auto otherBlock = sepString(headers, headerBlockSeparator, headTermSig, false, true);
			if (fileBlock == "NOT_FOUND" || otherBlock.size() == 0)
				return parsedHeaders; // invalid header - a header requires both a file block AND an 'other' block for SUFTP

			auto filesInHeaders = sepString(fileBlock, headFileStrt, headFileTerm);

			for (auto& file : filesInHeaders) {
				pair<std::string, std::vector<pair<std::string, std::string>>> thisFile;
				std::vector<pair<std::string, std::string>> theseHeaders;
				file += "\n";
				auto argumentsForFile = sepString(file, headerTitleStrt, "\n", false, false);

				for (auto& arg : argumentsForFile) {
					auto title = singleSearch(arg, headerTitleStrt, headAssociativeOperator);
					auto value = singleSearch(arg, headAssociativeOperator, "\n");
					if (title == "FPATH")
						thisFile.first = ("FILE");
					theseHeaders.push_back(pair<std::string, std::string>(title, value));
				}
				thisFile.second = (theseHeaders);
				parsedHeaders.push_back(thisFile);
			}

			for (auto& other : otherBlock) {
				std::string headerName = singleSearch(other, headerTitleStrt, headAssociativeOperator);
				std::string headerValue = singleSearch(other, headAssociativeOperator, "\n");
				auto head = pair<std::string, std::vector< pair<std::string, std::string>>>();
				auto arg = std::vector<pair<std::string, std::string>>();
				arg.push_back(pair<std::string, std::string>(headerName, headerValue));
				head.first = (headerName);
				head.second = (arg);
				parsedHeaders.push_back(head);
			}
			return parsedHeaders;
		}

	private:
		std::string interfaceToBindTo;
		int portToBindTo;
		std::string rootDirectory;

	public slots:
		void incrementFilesTransferred(int f);
		void transferDone();
		void transferFailed(int code);
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class SUFTPSender_REMOTE : public QThread
	{
		Q_OBJECT
	public:
		SUFTPSender_REMOTE(std::string interfaceToBindTo, int portToBindTo, std::string rootDirectory, std::vector<std::string> filenames) {
			this->interfaceToBindTo = interfaceToBindTo;
			this->portToBindTo = portToBindTo;
			this->rootDirectory = rootDirectory;
			this->filenames = filenames;
			p_libsys_init();
			isRunning = false;
		}
		~SUFTPSender_REMOTE() {
			p_libsys_shutdown();
		}
		void run() override {
			if (isRunning)
				return;
			isRunning = true;
			int code = privateRun(interfaceToBindTo, portToBindTo, rootDirectory, filenames);
			isRunning = false;
		}

	protected:
		bool isRunning;
		int privateRun(std::string interfaceToBindTo, int portToBindTo, std::string rootDirectory, std::vector<std::string> filenames) {
			PSocketAddress* addr;
			PSocket* sock;

			// Construct address for server.  Since the server is assumed to be on the same machine for the sake of this program, the address is loopback, but typically this would be an external address.
			if ((addr = p_socket_address_new(interfaceToBindTo.c_str(), portToBindTo)) == NULL)
				emit transferFailed(1);

			// Create socket
			if ((sock = p_socket_new(P_SOCKET_FAMILY_INET, P_SOCKET_TYPE_STREAM, P_SOCKET_PROTOCOL_TCP, NULL)) == NULL)
			{
				// Can't construct socket, cleanup and exit.
				p_socket_address_free(addr);

				emit transferFailed(2);
			}
			// Connect to server.
			if (!p_socket_connect(sock, addr, NULL))
			{
				// Couldn't connect, cleanup.
				p_socket_address_free(addr);
				p_socket_free(sock);
				emit transferFailed(4);
			}

			char* raw = new char[(MAX_MESSAGE_LENGTH + 2)];
			std::string headers;


			char* headerBuffer = new char[MAX_MESSAGE_LENGTH + 2];
			for (int i = 0; i < MAX_MESSAGE_LENGTH; i++) {
				headerBuffer[i] = 0;
			}

			// Forever, try to accept incoming connections.
			char garbage[4];
			char* buffer = new char[MAX_MESSAGE_LENGTH + 1]; // Supports messages up to max length (plus null character that terminates the std::string, since we're sending text)

			// We have a connection to the server, now do something about it.


			//headers = "00";

			//for (int i = 0; ; i++) {
			//	if (i % MAX_MESSAGE_LENGTH == 0) {
			//		p_socket_send(sock, headerBuffer, MAX_MESSAGE_LENGTH, NULL);
			//		//p_socket_receive(sock, garbage, sizeof(garbage), NULL);
			//	}
			//	else if (i == headers.size() - 1) {
			//		p_socket_send(sock, headerBuffer, i % MAX_MESSAGE_LENGTH, NULL);
			//		//p_socket_receive(sock, garbage, sizeof(garbage), NULL);
			//		break;
			//	}
			//	headerBuffer[i % MAX_MESSAGE_LENGTH] = headers[i];
			//}
			////p_socket_send(sock, headTermSig.c_str(), sizeof(headTermSig) + 1, NULL);

			p_socket_receive(sock, headerBuffer, MAX_MESSAGE_LENGTH, NULL);

			{
				// Continue with normal SUFTP protocol

				std::ifstream file;
				std::string headers;

				for (auto& fname : filenames) {
					size_t invalid = -1;

					file.open(fname, std::ios::binary | std::ios::ate);
					size_t filesize = file.tellg();
					file.close();
					if (filesize == invalid)
						continue;
					headers += headFileStrt;
					std::string relativePath = fname;
					size_t i = fname.find(rootDirectory);
					if (i != std::string::npos) {
						relativePath.replace(i, rootDirectory.size(), "");
						headers += createHeader("FPATH", relativePath);
						headers += createHeader("FPATHTYPE", "REL");
					}
					else {
						headers += createHeader("FPATH", relativePath);
						headers += createHeader("FPATHTYPE", "ABS");
					}
					headers += createHeader("FSIZE", std::to_string(filesize));
					headers += headFileTerm;
				}

				headers += headerBlockSeparator;

				std::vector<pair<std::string, std::string>> otherHeaders;
				otherHeaders.push_back(pair<std::string, std::string>("USERID", "PREALPHA_stuff"));
				for (auto& otherHeader : otherHeaders) {
					headers += createHeader(otherHeader.first, otherHeader.second);
				}

				char* headerBuffer = new char[MAX_MESSAGE_LENGTH + 1];
				for (int i = 0; i < MAX_MESSAGE_LENGTH; i++) {
					headerBuffer[i] = 0;
				}

				// send headers
				for (int i = 0; ; i++) {
					if (i % MAX_MESSAGE_LENGTH == 0) {
						p_socket_send(sock, headerBuffer, MAX_MESSAGE_LENGTH, NULL);
						p_socket_receive(sock, garbage, sizeof(garbage), NULL);
					}
					else if (i == headers.size() - 1) {
						p_socket_send(sock, headerBuffer, i % MAX_MESSAGE_LENGTH, NULL);
						p_socket_receive(sock, garbage, sizeof(garbage), NULL);
						break;
					}
					headerBuffer[i % MAX_MESSAGE_LENGTH] = headers[i];
				}
				p_socket_send(sock, headTermSig.c_str(), sizeof(headTermSig) + 1, NULL);
				p_socket_receive(sock, garbage, sizeof(garbage), NULL);

				// block until recipient is ready for the files themselves
				p_socket_receive(sock, garbage, sizeof(garbage), NULL);

				size_t f = 0;

				for (auto& filename : filenames) {
					if (filename == "")
						continue;
					file.open(filename, std::ios::binary);
					while (!file.eof()) {
						int i = 0;
						for (i = 0; (file.good() && i < MAX_MESSAGE_LENGTH); i++) {
							buffer[i] = file.get();
							if (i != MAX_MESSAGE_LENGTH - 1)
								buffer[i + 1] = 0;
						}

						if (sock != NULL)
						{
							p_socket_send(sock, buffer, MAX_MESSAGE_LENGTH, NULL);
						}
						else
							printf("Can't make con, tried and failed...\n");
					}
					file.close();
					emit incrementFilesTransferred(f);
					std::cout << "sent" << std::endl;
				}

				p_socket_send(sock, streamTermSig.c_str(), sizeof(streamTermSig.c_str()) + 1, NULL);
				p_socket_close(sock, NULL);

				delete[] buffer;
				// Cleanup
				p_socket_address_free(addr);
				p_socket_close(sock, NULL);
				emit transferDone();

			}

			return 0;
		}
		static std::string createHeader(const std::string& title, const std::string& value) {
			std::string header = headerTitleStrt;

			for (int i = 0; i < title.size(); i++) {
				char upper = toupper(title[i]);
				if (strchr(alpha, upper))
					header += toupper(title[i]);
			}

			header += headAssociativeOperator + value + "\n";
			return header;
		}

	private:
		std::string interfaceToBindTo;
		int portToBindTo;
		std::string rootDirectory;
		std::vector<std::string> filenames;

	public slots:
		void incrementFilesTransferred(int f);
		void transferDone();
		void transferFailed(int code);
	};

	class SUFTPReceiver_REMOTE : public QThread
	{
		Q_OBJECT
	public:
		SUFTPReceiver_REMOTE(std::string interfaceToBindTo, int portToBindTo, std::string rootDirectory) {
			this->interfaceToBindTo = interfaceToBindTo;
			this->portToBindTo = portToBindTo;
			this->rootDirectory = rootDirectory;
			p_libsys_init();
			isRunning = false;
		}
		~SUFTPReceiver_REMOTE() {
			p_libsys_shutdown();
		}
		
		void run() override {
			if (isRunning)
				return;
			isRunning = true;
			int code = privateRun(interfaceToBindTo, portToBindTo, rootDirectory);
			isRunning = false;
		}

	protected:
		bool isRunning;
		int privateRun(std::string interfaceToBindTo, int portToBindTo, std::string rootDirectory) {
			PSocketAddress* addr;
			PSocket* sock;

			// Construct address for server.  Since the server is assumed to be on the same machine for the sake of this program, the address is loopback, but typically this would be an external address.
			if ((addr = p_socket_address_new(interfaceToBindTo.c_str(), portToBindTo)) == NULL)
				emit transferFailed(1);

			// Create socket
			if ((sock = p_socket_new(P_SOCKET_FAMILY_INET, P_SOCKET_TYPE_STREAM, P_SOCKET_PROTOCOL_TCP, NULL)) == NULL)
			{
				// Can't construct socket, cleanup and exit.
				p_socket_address_free(addr);

				emit transferFailed(2);
			}
			// Connect to server.
			if (!p_socket_connect(sock, addr, NULL))
			{
				// Couldn't connect, cleanup.
				p_socket_address_free(addr);
				p_socket_free(sock);
				emit transferFailed(4);
			}

			char* raw = new char[(MAX_MESSAGE_LENGTH + 2)];
			char* headerBuffer = new char[MAX_MESSAGE_LENGTH + 2];
			std::string headers;
			for (int i = 0; i < MAX_MESSAGE_LENGTH; i++) {
				headerBuffer[i] = 0;
			}


			// We now have a connection to the server, so do something about it


			//headers = "00";
			//size_t size = headers.size();

			//for (int i = 0; ; i++) {
			//	if (i % MAX_MESSAGE_LENGTH == 0) {
			//		p_socket_send(sock, headerBuffer, MAX_MESSAGE_LENGTH, NULL);
			//	}
			//	else if (i == size-1) {
			//		p_socket_send(sock, headerBuffer, i % MAX_MESSAGE_LENGTH, NULL);
			//		break;
			//	}
			//	headerBuffer[i % MAX_MESSAGE_LENGTH] = headers[i];
			//}
			////p_socket_send(sock, headTermSig.c_str(), sizeof(headTermSig) + 1, NULL);
			p_socket_receive(sock, headerBuffer, MAX_MESSAGE_LENGTH, NULL);




			// Continue with normal SUFTP protocol
			{


				std::string headers;


				for (int i = 0; i < MAX_MESSAGE_LENGTH; i++) {
					headerBuffer[i] = 0;
				}

				while (true) {
					pssize sizeOfRecvData = p_socket_receive(sock, headerBuffer, MAX_MESSAGE_LENGTH, NULL);
					headerBuffer[MAX_MESSAGE_LENGTH + 1] = '\0';
					p_socket_send(sock, "R", 1, NULL);
					if (strstr(headerBuffer, headTermSig.c_str()) != nullptr)
						break;
					headers += headerBuffer;

				}

				auto q = parseHeaders(headers);
				std::cout << headers << std::endl;
				std::vector<Traversal> filesToReceive;

				for (auto& i : q) {
					if (i.first == "FILE") {
						std::vector<pair<std::string, std::string>> w = i.second;
						Traversal r(w);
						filesToReceive.push_back(r);
					}
				}

				bool streamDone = false;

				// Signal the sender that we're ready for all files to be sent in one blob
				p_socket_send(sock, "R", 1, NULL);

				size_t totalContentPacketsExpected = 0;
				for (auto& i : filesToReceive) {
					int expectedPackets = stoi(i["FSIZE"]);
					expectedPackets = (expectedPackets / MAX_MESSAGE_LENGTH);
					totalContentPacketsExpected += expectedPackets;
				}

				size_t totalPacketsReceived = 0;
				size_t f = 0;

				for (auto& currentFileInfo : filesToReceive) {
					std::string filename = rootDirectory + currentFileInfo["FPATH"];

					int expectedPackets = stoi(currentFileInfo["FSIZE"]);
					int expectedFinalPacketLength = expectedPackets % MAX_MESSAGE_LENGTH;
					expectedPackets = (expectedPackets / MAX_MESSAGE_LENGTH);
					/* In this next block, we're making sure that the parent path for the given file exists.
					If not, make it. */
					boost::filesystem::path fileAsPath(filename);
					if (!boost::filesystem::exists(fileAsPath.parent_path())) {
						try {
							boost::filesystem::create_directories(fileAsPath.parent_path());
						}
						catch (boost::filesystem::filesystem_error e) {
							cout << e.code() << endl;
							cout << e.path1() << endl;
							cout << e.what() << endl;
						}
					}


					std::ofstream fileOut;
					fileOut.open(filename, std::ios::binary);

					for (int packets = 0; ; packets++) {
						pssize sizeOfRecvData = p_socket_receive(sock, raw, MAX_MESSAGE_LENGTH, NULL);
						totalPacketsReceived++;
						raw[sizeOfRecvData] = '\0'; // Set null character 1 after message
						if (totalPacketsReceived == totalContentPacketsExpected+1) {
							streamDone = true;
							break;
						}

						if (packets == expectedPackets) {
							for (int bits = 0; bits < expectedFinalPacketLength; bits++)
								fileOut << raw[bits];
							break;
						}
						else
							fileOut.write(raw, MAX_MESSAGE_LENGTH);

					}
					fileOut.close();
					f++;
					emit incrementFilesTransferred(f);
					if (streamDone)
						break;
				}

				delete[] raw;
				p_socket_address_free(addr);
				p_socket_close(sock, NULL);
			}
			emit transferDone();
			return 0;
		}
		static std::vector<pair<std::string, std::vector<pair<std::string, std::string>>>> parseHeaders(std::string headers) {
			std::vector<pair<std::string, std::vector<pair<std::string, std::string>>>> parsedHeaders;
			auto fileBlock = singleSearch(headers, headFileStrt, headerBlockSeparator, false, false);
			auto otherBlock = sepString(headers, headerBlockSeparator, headTermSig, false, true);
			if (fileBlock == "NOT_FOUND" || otherBlock.size() == 0)
				return parsedHeaders; // invalid header - a header requires both a file block AND an 'other' block for SUFTP

			auto filesInHeaders = sepString(fileBlock, headFileStrt, headFileTerm);

			for (auto& file : filesInHeaders) {
				pair<std::string, std::vector<pair<std::string, std::string>>> thisFile;
				std::vector<pair<std::string, std::string>> theseHeaders;
				file += "\n";
				auto argumentsForFile = sepString(file, headerTitleStrt, "\n", false, false);

				for (auto& arg : argumentsForFile) {
					auto title = singleSearch(arg, headerTitleStrt, headAssociativeOperator);
					auto value = singleSearch(arg, headAssociativeOperator, "\n");
					if (title == "FPATH")
						thisFile.first = ("FILE");
					theseHeaders.push_back(pair<std::string, std::string>(title, value));
				}
				thisFile.second = (theseHeaders);
				parsedHeaders.push_back(thisFile);
			}

			for (auto& other : otherBlock) {
				std::string headerName = singleSearch(other, headerTitleStrt, headAssociativeOperator);
				std::string headerValue = singleSearch(other, headAssociativeOperator, "\n");
				auto head = pair<std::string, std::vector< pair<std::string, std::string>>>();
				auto arg = std::vector<pair<std::string, std::string>>();
				arg.push_back(pair<std::string, std::string>(headerName, headerValue));
				head.first = (headerName);
				head.second = (arg);
				parsedHeaders.push_back(head);
			}
			return parsedHeaders;
		}
	
	private:
			std::string interfaceToBindTo;
			int portToBindTo;
			std::string rootDirectory;

	public slots:
		void incrementFilesTransferred(int f);
		void transferDone();
		void transferFailed(int code);
	};

}