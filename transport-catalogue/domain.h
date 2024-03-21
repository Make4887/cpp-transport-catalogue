#pragma once
#include <string>
#include <string_view>
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

struct EdgeInfo {
	std::string_view name;
	double time;
	int span_count = 0;
};

struct RouteInfo {
	double all_time;
	std::vector<EdgeInfo> edges;
};