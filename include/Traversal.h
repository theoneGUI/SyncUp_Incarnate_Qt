#pragma once
#include <vector>
#include <string>
#include <utility>
#include <QString>

class Traversal {
public:
	Traversal(std::vector<std::pair<std::string, std::string>>& input)
		: values{ input }
	{
	}
	std::string operator[](std::string& search)
	{
		for (std::pair<std::string, std::string>& item : values) {
			if (item.first == search)
				return item.second;
		}
		return "NOT_FOUND";
	}
	std::string operator[](const char* search)
	{
		for (std::pair<std::string, std::string>& item : values) {
			if (item.first == search)
				return item.second;
		}
		return "NOT_FOUND";
	}
	std::string operator[](QString& s)
	{
		std::string search = s.toStdString();
		for (std::pair<std::string, std::string>& item : values) {
			if (item.first == search)
				return item.second;
		}
		return "NOT_FOUND";
	}
	size_t size() const { return values.size(); };
	std::vector<std::string> keys()
	{
		std::vector<std::string> out;
		for (auto& i : values) {
			out.push_back(i.first);
		}
		return out;
	}
	std::vector<std::pair<std::string, std::string>> reveal() {
		return values;
	}
	void set(std::string at, std::string value) {
		for (std::pair<std::string, std::string>& item : values) {
			if (item.first == at)
				item.second = value;
		}
	}

private:
	std::vector<std::pair<std::string, std::string>> values;
};
