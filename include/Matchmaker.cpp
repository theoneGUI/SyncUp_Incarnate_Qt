#include "Matchmaker.h"

std::string exec(const char* cmd) {
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
	if (!pipe) {
		throw std::runtime_error("popen failed");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
		result += buffer.data();
	return result;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

std::string singleSearch(const std::string& str, const std::string& beginning, const std::string& end, bool truncEnd, bool truncBegin) {
	std::string out;
	auto occurence = str.find(beginning);
	if (occurence == std::string::npos)
		return "NOT_FOUND";
	auto term = str.find(end, occurence);

	if (truncEnd && truncBegin) {
		int len = occurence - term;
		len = abs(len) - 1;

		return(str.substr(occurence + beginning.size(), len));
	}
	else if (truncEnd && !truncBegin) {
		int len = occurence - term;
		len = abs(len);

		return(str.substr(occurence, len));
	}
	else if (!truncEnd && truncBegin) {
		int len = occurence - term;
		len = abs(len);

		return(str.substr(occurence + beginning.size(), len));
	}
	else /* truncate neither end nor begin */ {
		int len = occurence - term;
		len = abs(len) + end.size();

		return(str.substr(occurence, len));
	}

}

std::vector<std::string> sepString(const std::string& str, const std::string& beginning, const std::string& end, bool truncEnd, bool truncBegin) {
	int offset = 0;
	std::vector<std::string> out;
	while (true) {
		auto occurence = str.find(beginning, offset);
		if (occurence == std::string::npos)
			break;
		auto term = str.find(end, occurence);

		if (truncEnd && truncBegin) {
			int len = occurence - term;
			len = abs(len) - 1;

			out.push_back(str.substr(occurence + beginning.size(), len));
		}
		else if (truncEnd && !truncBegin) {
			int len = occurence - term;
			len = abs(len);

			out.push_back(str.substr(occurence, len));
		}
		else if (!truncEnd && truncBegin) {
			int len = occurence - term;
			len = abs(len);

			out.push_back(str.substr(occurence + beginning.size(), len));
		}
		else /* truncate neither end nor begin */ {
			int len = occurence - term;
			len = abs(len) + end.size();

			out.push_back(str.substr(occurence, len));
		}
		if (offset + term < offset)
			break;
		offset = term;
	}
	return out;
}

std::string getPublicIP() {
	CURL* curl;
	CURLcode res;
	std::string out;
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "http://myexternalip.com/raw");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	return out;
}

std::vector<std::string> getAllIPs() {
#ifdef _WIN32
	auto raw = exec("ipconfig");
	std::vector<std::string> ipv4s;
	size_t offset = 0;
	std::string tofind = "IPv4 Address. . . . . . . . . . . : ";
	while (true) {
		size_t index = raw.find(tofind, offset + 1);
		size_t endline = raw.find("\n", index + 1);
		if (index == std::string::npos) {
			break;
		}
		else {
			ipv4s.push_back(raw.substr(index + tofind.size(), endline - index - tofind.size()));
		}
		offset = index + 1;
	}
#endif
#ifdef linux
	auto raw = exec("hostname -I");
	std::vector<std::string> ipv4s;
	size_t offset = 0;
	std::string tofind = " ";
	while (true) {
		size_t index = raw.find(tofind, offset + 1);
		size_t endline = raw.find(" ", index + 1);
		if (index == std::string::npos) {
			break;
		}
		else {
			ipv4s.push_back(raw.substr(index + tofind.size(), endline - index - tofind.size()));
		}
		offset = index + 1;
	}
#endif
	return ipv4s;
}

