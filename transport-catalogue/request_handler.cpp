#include "request_handler.h"

#include <set>
#include <vector>

RequestHandler::RequestHandler(const transport_catalogue::TransportCatalogue& db, const map_renderer::MapRenderer& renderer) 
	: db_(db)
	, renderer_(renderer)
{
}

void RequestHandler::AddBus(svg::Document& doc, svg::Point screen_coord,
								std::string_view bus, size_t index) const {
	svg::Text bus_name;
	svg::Text bus_name_backing;
	bus_name.SetPosition(screen_coord);
	bus_name_backing.SetPosition(screen_coord);

	bus_name.SetData(std::string(bus));
	bus_name_backing.SetData(std::string(bus));

	renderer_.AddBusSettings(bus_name, index);
	renderer_.AddBusBackingSettings(bus_name_backing);

	doc.Add(bus_name_backing);
	doc.Add(bus_name);
}

void RequestHandler::AddStop(svg::Document& doc, svg::Point screen_coord, std::string_view stop) const {
	svg::Text stop_name;
	svg::Text stop_name_backing;
	stop_name.SetPosition(screen_coord);
	stop_name_backing.SetPosition(screen_coord);

	stop_name.SetData(std::string(stop));
	stop_name_backing.SetData(std::string(stop));

	renderer_.AddStopSettings(stop_name);
	renderer_.AddStopBackingSettings(stop_name_backing);

	doc.Add(stop_name_backing);
	doc.Add(stop_name);
}

void RequestHandler::RenderMap(std::ostream& output) const {
	std::vector<geo::Coordinates> all_geo_coords;
	for (const auto bus : renderer_.FilterBuses(db_.GetAllBuses())) {
		for (const Stop* stop : db_.FindBus(bus)->route) {
			all_geo_coords.push_back(stop->coordinates);
		}
	}
	const map_renderer::SphereProjector proj = renderer_.CreateSphereProjector(all_geo_coords.begin(), all_geo_coords.end());
	
	svg::Document doc;
	std::set<std::string_view> all_stops;
	size_t index = 0;
	for (const auto bus : renderer_.FilterBuses(db_.GetAllBuses())) {
		svg::Polyline rout;
		renderer_.AddRoutSettings(rout, index);
		for (const Stop* stop : db_.FindBus(bus)->route) {
			rout.AddPoint(proj(stop->coordinates));
			all_stops.insert(stop->name);
		}
		doc.Add(rout);
		++index;
	}

	index = 0;
	for (const auto bus : renderer_.FilterBuses(db_.GetAllBuses())) {
		const auto& bus_stops = db_.FindBus(bus)->route;
		AddBus(doc, proj(bus_stops.front()->coordinates), bus, index);

		if (!db_.FindBus(bus)->ring && bus_stops.front() != bus_stops[bus_stops.size() / 2]) {
			AddBus(doc, proj(bus_stops[bus_stops.size() / 2]->coordinates), bus, index);
		}

		++index;
	}

	for (const auto stop : all_stops) {
		svg::Circle stop_symbol;
		stop_symbol.SetCenter(proj(db_.FindStop(stop)->coordinates));
		renderer_.AddStopSymbolSettings(stop_symbol);
		doc.Add(stop_symbol);
	}

	for (const auto stop : all_stops) {
		AddStop(doc, proj(db_.FindStop(stop)->coordinates), stop);
	}

	doc.Render(output);
}