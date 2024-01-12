#include <unordered_set>
#include <utility>

#include "transport_catalogue.h"

namespace transportcatalogue {

void TransportCatalogue::AddStop(const std::string& name, geo::Coordinates coordinates) {
	stops_.push_back({ name, coordinates });
	stopname_to_stop_[stops_.back().name] = &stops_.back();
}

void TransportCatalogue::AddBus(const std::string& name, const std::vector<std::string_view>& str_route) {
	std::vector<Stop*> route(str_route.size());
	for (int i = 0; i < static_cast<int>(route.size()); ++i) {
		route[i] = stopname_to_stop_.at(str_route[i]);
	}
	buses_.push_back({ name, std::move(route) });
	busname_to_bus_[buses_.back().name] = &buses_.back();
	for (auto stop: str_route) {
		stop_to_busnames_[stopname_to_stop_.at(stop)].insert(busname_to_bus_.at(name)->name);
	}
}

bool TransportCatalogue::FindBus(std::string_view name) const {
	return busname_to_bus_.count(name);
}

bool TransportCatalogue::FindStop(std::string_view name) const {
	return stopname_to_stop_.count(name);
}

size_t TransportCatalogue::CountBusStops(const std::string& name) const {
	return busname_to_bus_.at(name)->route.size();
}

size_t TransportCatalogue::CountUniqueBusStops(const std::string& name) const {
	const std::vector<Stop*>& route = busname_to_bus_.at(name)->route;
	const std::unordered_set<Stop*> unique_stops(route.begin(), route.end());
	return unique_stops.size();
}

double TransportCatalogue::ComputeRouteLength(const std::string& name) const {
	const std::vector<Stop*>& route = busname_to_bus_.at(name)->route;
	double result = 0.;
	for (int i = 1; i < static_cast<int>(route.size()); ++i) {
		result += geo::ComputeDistance(route[i - 1]->coordinates, route[i]->coordinates);
	}
	return result;
}

std::set<std::string_view> TransportCatalogue::GetBusesPassingThroughStop(std::string_view stop) const {
	if (!stop_to_busnames_.count(stopname_to_stop_.at(stop))) {
		return {};
	}
	return stop_to_busnames_.at(stopname_to_stop_.at(stop));
}
}