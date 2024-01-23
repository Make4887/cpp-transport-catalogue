#include <unordered_set>
#include <utility>

#include "transport_catalogue.h"

namespace transport_catalogue {

void TransportCatalogue::AddStop(const std::string& name, geo::Coordinates coordinates) {
	stops_.push_back({ name, coordinates });
	stopname_to_stop_[stops_.back().name] = &stops_.back();
}

void TransportCatalogue::AddDistances(std::string_view main_name, std::string_view neighbour_name, int distance) {
	Stop* main_stop = stopname_to_stop_.at(main_name);
	Stop* neighbour_stop = stopname_to_stop_.at(neighbour_name);
	distance_between_stops_[{main_stop, neighbour_stop}] = distance;
	if (distance_between_stops_.count({ neighbour_stop, main_stop }) == 0) {
		distance_between_stops_[{neighbour_stop, main_stop}] = distance;
	}
}

void TransportCatalogue::AddBus(const std::string& name, const std::vector<std::string_view>& str_route) {
	std::vector<Stop*> route(str_route.size());
	for (int i = 0; i < static_cast<int>(route.size()); ++i) {
		route[i] = stopname_to_stop_.at(str_route[i]);
	}
	buses_.push_back({ name, std::move(route) });
	busname_to_bus_[buses_.back().name] = &buses_.back();
	for (auto stop : str_route) {
		stop_to_busnames_[stopname_to_stop_.at(stop)].insert(busname_to_bus_.at(name)->name);
	}
}

TransportCatalogue::Bus* TransportCatalogue::FindBus(std::string_view name) const {
	if (busname_to_bus_.count(name)) {
		return busname_to_bus_.at(name);
	}
	return nullptr;
}

TransportCatalogue::Stop* TransportCatalogue::FindStop(std::string_view name) const {
	if (stopname_to_stop_.count(name)) {
		return stopname_to_stop_.at(name);
	}
	return nullptr;
}

int TransportCatalogue::GetDistance(std::string_view main_name, std::string_view neighbour_name) const {
	return distance_between_stops_.at({ stopname_to_stop_.at(main_name), stopname_to_stop_.at(neighbour_name) });
}

TransportCatalogue::BusInfo TransportCatalogue::GetBusInfo(std::string_view bus) const {
	const size_t count_all_stops = busname_to_bus_.at(bus)->route.size();

	const std::vector<Stop*>& route = busname_to_bus_.at(bus)->route;
	const std::unordered_set<Stop*> unique_stops(route.begin(), route.end());
	const size_t count_unique_stops = unique_stops.size();

	int route_lenght = 0;
	for (int i = 1; i < static_cast<int>(route.size()); ++i) {
		route_lenght += distance_between_stops_.at({ route[i - 1], route[i] });
	}

	double geo_route_lenght = 0.;
	for (int i = 1; i < static_cast<int>(route.size()); ++i) {
		geo_route_lenght += geo::ComputeDistance(route[i - 1]->coordinates, route[i]->coordinates);
	}
	return { count_all_stops, count_unique_stops, route_lenght, route_lenght / geo_route_lenght };
}

std::set<std::string_view> TransportCatalogue::GetBusesPassingThroughStop(std::string_view stop) const {
	if (!stop_to_busnames_.count(stopname_to_stop_.at(stop))) {
		return {};
	}
	return stop_to_busnames_.at(stopname_to_stop_.at(stop));
}
}