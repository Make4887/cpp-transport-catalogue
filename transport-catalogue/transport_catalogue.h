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

	struct Stop {
		std::string name;
		geo::Coordinates coordinates;
	};

	struct Bus {
		std::string name;
		std::vector<Stop*> route;
	};

	struct BusInfo {
		size_t count_all_stops;
		size_t count_unique_stops;
		double route_lenght;
	};

public:
	void AddStop(const std::string& name, geo::Coordinates coordinates);

	void AddBus(const std::string& name, const std::vector<std::string_view>& str_route);

	Bus* FindBus(std::string_view name) const;

	Stop* FindStop(std::string_view name) const;

	BusInfo GetBusInfo(std::string_view name) const;

	std::set<std::string_view> GetBusesPassingThroughStop(std::string_view stop) const;

private:

	std::deque<Stop> stops_;
	std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
	std::deque<Bus> buses_;
	std::unordered_map<std::string_view, Bus*> busname_to_bus_;
	std::unordered_map<Stop*, std::set<std::string_view>> stop_to_busnames_;
};
}