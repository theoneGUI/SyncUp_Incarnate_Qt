#pragma once
#include <string>
#include <utility>

class Pointer {
public:
	Pointer();
	Pointer(long long unsigned& pt, std::pair<std::string, std::pair<std::string, std::string>>& is);
	long long unsigned pointsTo();
	std::pair<std::string, std::pair<std::string, std::string>> is();
	bool operator<(Pointer& comp);
	bool operator>(Pointer& comp);

private:
	std::pair<std::string, std::pair<std::string, std::string>> linkedObject;
	long long unsigned pointer;
};
