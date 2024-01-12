#pragma once
#include <deque>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "geo.h"

namespace transportcatalogue {

class TransportCatalogue {
public:
	void AddStop(const std::string& name, geo::Coordinates coordinates);

	void AddBus(const std::string& name, const std::vector<std::string_view>& str_route);

	bool FindBus(std::string_view name) const;

	bool FindStop(std::string_view name) const;

	size_t CountBusStops(const std::string& name) const;

	size_t CountUniqueBusStops(const std::string& name) const;

	double ComputeRouteLength(const std::string& name) const;

	std::set<std::string_view> GetBusesPassingThroughStop(std::string_view stop) const;

private:
	struct Stop {
		std::string name;
		geo::Coordinates coordinates;
	};

	struct Bus {
		std::string name;
		std::vector<Stop*> route;
	};

	std::deque<Stop> stops_;
	std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
	std::deque<Bus> buses_;
	std::unordered_map<std::string_view, Bus*> busname_to_bus_;
	std::unordered_map<Stop*, std::set<std::string_view>> stop_to_busnames_;
};
}