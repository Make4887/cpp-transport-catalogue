#include <unordered_set>
#include <utility>
#include <vector>

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

void TransportCatalogue::AddBus(const std::string& name, const std::vector<std::string_view>& str_route, bool ring) {
	std::vector<Stop*> route(str_route.size());
	for (int i = 0; i < static_cast<int>(route.size()); ++i) {
		route[i] = stopname_to_stop_.at(str_route[i]);
	}
	buses_.push_back({ name, std::move(route) , ring });
	busname_to_bus_[buses_.back().name] = &buses_.back();
	for (auto stop : str_route) {
		stop_to_busnames_[stopname_to_stop_.at(stop)].insert(busname_to_bus_.at(name)->name);
	}
}

void TransportCatalogue::AddRoutingSettings(double bus_velocity, int bus_wait_time) {
	bus_velocity_ = bus_velocity;
	bus_wait_time_ = bus_wait_time;
}

Bus* TransportCatalogue::FindBus(std::string_view name) const {
	if (busname_to_bus_.count(name)) {
		return busname_to_bus_.at(name);
	}
	return nullptr;
}

Stop* TransportCatalogue::FindStop(std::string_view name) const {
	if (stopname_to_stop_.count(name)) {
		return stopname_to_stop_.at(name);
	}
	return nullptr;
}

const std::deque<Bus>& TransportCatalogue::GetAllBuses() const {
	return buses_;
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

std::optional<RouteInfo> TransportCatalogue::GetRouteInfo(std::string_view from, std::string_view to, const graph::Router<double>& router) const {
	using namespace std::literals::string_literals;
	auto rout_info = router.BuildRoute(stopname_to_vertex_.at(from).first, stopname_to_vertex_.at(to).first);
	if (!rout_info.has_value()) {
		return std::nullopt;
	}
	RouteInfo result;
	result.all_time = rout_info->weight;
	for (graph::EdgeId edge_id: rout_info->edges) {
		result.edges.push_back(edge_id_to_edge_.at(static_cast<int>(edge_id)));
	}
	return result;
}

const graph::DirectedWeightedGraph<double>& TransportCatalogue::GetGraph() const {
	return graph_;
}

void TransportCatalogue::CreateGraph() {
	graph_ = graph::DirectedWeightedGraph<double>(2 * stops_.size());
	graph::VertexId ind_vertex = 0;
	for (const Stop& stop : stops_) {
		stopname_to_vertex_[stop.name] = { ind_vertex, ind_vertex + 1 };
		graph::Edge<double> edge;
		edge.from = ind_vertex;
		edge.to = ind_vertex + 1;
		edge.weight = bus_wait_time_;
		edge_id_to_edge_[static_cast<int>(graph_.AddEdge(edge))] = { stop.name, static_cast<double>(bus_wait_time_) };
		ind_vertex += 2;
	}
	for (const Bus& bus : buses_) {
		if (bus.ring) {
			for (int i = 0; i < static_cast<int>(bus.route.size()); ++i) {
				int route_lenght = 0;
				for (int j = i + 1; j < static_cast<int>(bus.route.size()); ++j) {
					if (bus.route[i]->name == bus.route[j]->name) {
						continue;
					}
					graph::Edge<double> edge;
					edge.from = stopname_to_vertex_[bus.route[i]->name].second;
					edge.to = stopname_to_vertex_[bus.route[j]->name].first;
					route_lenght += distance_between_stops_.at({ bus.route[j - 1], bus.route[j] });
					edge.weight = route_lenght / bus_velocity_ * 60. / 1000.;
					edge_id_to_edge_[static_cast<int>(graph_.AddEdge(edge))] = { bus.name, edge.weight, j - i };
				}
			}
		}
		else {
			for (int i = 0; i < static_cast<int>(bus.route.size()) / 2; ++i) {
				int route_lenght = 0;
				for (int j = i + 1; j <= static_cast<int>(bus.route.size()) / 2; ++j) {
					graph::Edge<double> edge;
					edge.from = stopname_to_vertex_[bus.route[i]->name].second;
					edge.to = stopname_to_vertex_[bus.route[j]->name].first;
					route_lenght += distance_between_stops_.at({ bus.route[j-1], bus.route[j] });
					edge.weight = route_lenght / bus_velocity_ * 60. / 1000.;
					edge_id_to_edge_[static_cast<int>(graph_.AddEdge(edge))] = { bus.name, edge.weight, j - i };
				}
			}
			for (int i = static_cast<int>(bus.route.size()) / 2; i < static_cast<int>(bus.route.size()); ++i) {
				int route_lenght = 0;
				for (int j = i + 1; j < static_cast<int>(bus.route.size()); ++j) {
					graph::Edge<double> edge;
					edge.from = stopname_to_vertex_[bus.route[i]->name].second;
					edge.to = stopname_to_vertex_[bus.route[j]->name].first;
					route_lenght += distance_between_stops_.at({ bus.route[j - 1], bus.route[j] });
					edge.weight = route_lenght / bus_velocity_ * 60. / 1000.;
					edge_id_to_edge_[static_cast<int>(graph_.AddEdge(edge))] = { bus.name, edge.weight, j - i };
				}
			}
		}
	}
}
}