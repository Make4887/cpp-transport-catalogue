#pragma once
#include <string>
#include <vector>

#include "geo.h"

struct Stop {
	std::string name;
	geo::Coordinates coordinates;
};

struct Bus {
	std::string name;
	std::vector<Stop*> route;
	bool ring;
};