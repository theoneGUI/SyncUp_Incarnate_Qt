#include "Pointer.h"

bool Pointer::operator>(Pointer& comp) {
	return pointer > comp.pointsTo();
}

std::pair<std::string, std::pair<std::string, std::string>> Pointer::is() { return linkedObject; }

Pointer::Pointer() : pointer(0) {};


Pointer::Pointer(long long unsigned& pt, std::pair<std::string, std::pair<std::string, std::string>>& is)
	: linkedObject(is), pointer(pt)
{
}

bool Pointer::operator<(Pointer& comp) {
	return pointer < comp.pointsTo();
}

long long unsigned Pointer::pointsTo() { return pointer; };
