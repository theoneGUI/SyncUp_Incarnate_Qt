#pragma once

#include <string>

enum class Type {
	NONE,
	STRING,
	INT,
	CSTRING
};

class Returnable {
protected:
	const Type* const type = &internalType;
	Type internalType;
	virtual void ghost() const = 0;
	virtual ~Returnable() = 0;
};

class None : public Returnable {
public:
	None() {
		internalType = Type::NONE;
	}
	~None() {
		delete type;
	}

private:
	void ghost() const override {}
};

class String : public Returnable {
public:
	String(const std::string& s) {
		internalType = Type::STRING;
		internalValue = s;
	}
	const std::string* const value = &internalValue;

private:
	std::string internalValue;
	void ghost() const override {}
};

class Int : public Returnable {
public:
	Int(const int& s) {
		internalType = Type::INT;
		internalValue = s;
	}
	const int* const value = &internalValue;

private:
	int internalValue;
	void ghost() const override {}
};

class CString : public Returnable {
public:
	CString(char* s) {
		internalType = Type::CSTRING;
		internalValue = s;
	}
	const char* const value = internalValue;

private:
	char* internalValue;
	void ghost() const override {}
};