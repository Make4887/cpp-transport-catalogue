#pragma once
#include <deque>
#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "geo.h"

namespace transport_catalogue {

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
		int route_lenght;
		double curvature;
	};

	struct StopsHasher {
		size_t operator()(const std::pair<Stop*, Stop*>& stops) const {
			static const size_t N = 37;
			return bus_hasher_(stops.first) + N * bus_hasher_(stops.second);
		}
	private:
		std::hash<Stop*> bus_hasher_;
	};

public:
	void AddStop(const std::string& name, geo::Coordinates coordinates);

	void AddDistances(std::string_view main_name, std::string_view neighbour_name, int distance);

	void AddBus(const std::string& name, const std::vector<std::string_view>& str_route);

	Bus* FindBus(std::string_view name) const;

	Stop* FindStop(std::string_view name) const;

	int GetDistance(std::string_view main_name, std::string_view neighbour_name) const;

	BusInfo GetBusInfo(std::string_view name) const;

	std::set<std::string_view> GetBusesPassingThroughStop(std::string_view stop) const;

private:

	std::deque<Stop> stops_;
	std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
	std::deque<Bus> buses_;
	std::unordered_map<std::string_view, Bus*> busname_to_bus_;
	std::unordered_map<Stop*, std::set<std::string_view>> stop_to_busnames_;
	std::unordered_map<std::pair<Stop*, Stop*>, int, StopsHasher> distance_between_stops_;
};
}