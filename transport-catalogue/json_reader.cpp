#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "json_builder.h"
#include "json_reader.h"
#include "request_handler.h"

namespace json_reader {
namespace detail {

std::vector<std::string_view> Route(const json::Dict& command) {
    using namespace std::literals::string_literals;
    std::vector<std::string_view> result;
    const auto& stops = command.at("stops"s).AsArray();
    if (command.at("is_roundtrip"s).AsBool()) {
        for (const auto& stop : stops) {
            result.push_back(stop.AsString());
        }
    }
    else {
        for (const auto& stop : stops) {
            result.push_back(stop.AsString());
        }
        for (int i = static_cast<int>(stops.size()) - 2; i >= 0; --i) {
            result.push_back(stops[i].AsString());
        }
    }
    return result;
}

void AddBusInfo(const transport_catalogue::TransportCatalogue& catalogue, const json::Dict& command, json::Builder& builder) {
    using namespace std::literals::string_literals;
    using namespace json;
    if (catalogue.FindBus(command.at("name"s).AsString())) {
        auto bus_info = catalogue.GetBusInfo(command.at("name"s).AsString());
        builder.StartDict()
                    .Key("curvature"s).Value(bus_info.curvature)
                    .Key("request_id"s).Value(command.at("id"s).AsInt())
                    .Key("route_length"s).Value(bus_info.route_lenght)
                    .Key("stop_count"s).Value(static_cast<int>(bus_info.count_all_stops))
                    .Key("unique_stop_count"s).Value(static_cast<int>(bus_info.count_unique_stops))
                .EndDict();
    }
    else {
        builder.StartDict()
                    .Key("request_id"s).Value(command.at("id"s).AsInt())
                    .Key("error_message"s).Value("not found"s)
                .EndDict();
    }
}

void AddStopInfo(const transport_catalogue::TransportCatalogue& catalogue, const json::Dict& command, json::Builder& builder) {
    using namespace std::literals::string_literals;
    using namespace json;
    if (catalogue.FindStop(command.at("name"s).AsString())) {
        const std::set<std::string_view> buses = catalogue.GetBusesPassingThroughStop(command.at("name"s).AsString());
        Array ar;
        for (auto bus : buses) {
            ar.push_back(Node(std::string(bus)));
        }
        builder.StartDict()
                    .Key("buses"s).Value(ar)
                    .Key("request_id"s).Value(command.at("id"s).AsInt())
                .EndDict();
    }
    else {
        builder.StartDict()
                    .Key("request_id"s).Value(command.at("id"s).AsInt())
                    .Key("error_message"s).Value("not found"s)
                .EndDict();
    }
}

void AddMapInfo(const transport_catalogue::TransportCatalogue& catalogue, const map_renderer::MapRenderer& map_renderer,
                        const json::Dict& command, json::Builder& builder) {
    using namespace std::literals::string_literals;
    RequestHandler request_handler(catalogue, map_renderer);
    std::ostringstream buf_stream;
    request_handler.RenderMap(buf_stream);
    builder.StartDict()
                .Key("map"s).Value(buf_stream.str())
                .Key("request_id"s).Value(command.at("id"s).AsInt())
            .EndDict();
}

void AddRouteInfo(const transport_catalogue::TransportCatalogue& catalogue, const json::Dict& command, json::Builder& builder, const graph::Router<double>& router) {
    using namespace std::literals::string_literals;
    using namespace json;
    std::optional<RouteInfo> route_info = catalogue.GetRouteInfo(command.at("from"s).AsString(), command.at("to"s).AsString(), router);
    if (!route_info.has_value()) {
        builder.StartDict()
            .Key("request_id"s).Value(command.at("id"s).AsInt())
            .Key("error_message"s).Value("not found"s)
            .EndDict();
        return;
    }
    Array ar;
    for (const EdgeInfo& edge_info : route_info->edges) {
        Dict dict;
        if (edge_info.span_count == 0) {
            dict["type"s] = "Wait"s;
            dict["stop_name"s] = std::string(edge_info.name);
            dict["time"s] = edge_info.time;
        }
        else {
            dict["type"s] = "Bus"s;
            dict["bus"s] = std::string(edge_info.name);
            dict["span_count"s] = edge_info.span_count;
            dict["time"s] = edge_info.time;
        }
        ar.push_back(std::move(dict));
    }
    builder.StartDict()
        .Key("request_id"s).Value(command.at("id"s).AsInt())
        .Key("total_time"s).Value(route_info->all_time)
        .Key("items"s).Value(std::move(ar))
        .EndDict();
}

svg::Color GetColor(const json::Node& color) {
    if (color.IsString()) {
        return color.AsString();
    }
    else if (color.IsArray() && color.AsArray().size() == 3) {
        return svg::Rgb{static_cast<uint8_t>(color.AsArray()[0].AsInt()),
                        static_cast<uint8_t>(color.AsArray()[1].AsInt()),
                        static_cast<uint8_t>(color.AsArray()[2].AsInt()) };
    }
    else if (color.IsArray() && color.AsArray().size() == 4) {
        return svg::Rgba{ static_cast<uint8_t>(color.AsArray()[0].AsInt()),
                          static_cast<uint8_t>(color.AsArray()[1].AsInt()),
                          static_cast<uint8_t>(color.AsArray()[2].AsInt()),
                          color.AsArray()[3].AsDouble() };
    }
    else {
        return {};
    }
}

} // detail

JsonReader::JsonReader(std::istream& input)
    :document_(json::Load(input))
{
}

void JsonReader::ApplyBaseCommands([[maybe_unused]] transport_catalogue::TransportCatalogue& catalogue) const{
    using namespace std::literals::string_literals;
    for (const auto& command : document_.GetRoot().AsDict().at("base_requests"s).AsArray()) {
        const auto com = command.AsDict();

        if (com.at("type"s).AsString() == "Stop"s) {
            catalogue.AddStop(com.at("name"s).AsString(), geo::Coordinates{ com.at("latitude"s).AsDouble(), com.at("longitude"s).AsDouble() });
        }
    }

    for (const auto& command : document_.GetRoot().AsDict().at("base_requests"s).AsArray()) {
        const auto com = command.AsDict();

        if (com.at("type"s).AsString() == "Stop"s) {
            for (auto [neighbour_name, dist] : com.at("road_distances"s).AsDict()) {
                catalogue.AddDistances(com.at("name"s).AsString(), neighbour_name, dist.AsInt());
            }
        }
    }

    for (const auto& command : document_.GetRoot().AsDict().at("base_requests"s).AsArray()) {
        const auto com = command.AsDict();

        if (com.at("type"s).AsString() == "Bus"s) {
            catalogue.AddBus(com.at("name"s).AsString(), detail::Route(com), com.at("is_roundtrip"s).AsBool());
        }
    }
}

void JsonReader::AddRoutingSettings(transport_catalogue::TransportCatalogue& catalogue) const {
    using namespace std::literals::string_literals;
    const json::Dict& settings_map = document_.GetRoot().AsDict().at("routing_settings"s).AsDict();
    catalogue.AddRoutingSettings(settings_map.at("bus_velocity"s).AsDouble(), settings_map.at("bus_wait_time"s).AsInt());
    catalogue.CreateGraph();
}

void JsonReader::ApplyStatCommands(const transport_catalogue::TransportCatalogue& catalogue,
    const map_renderer::MapRenderer& map_renderer, std::ostream& output) const {
    using namespace std::literals::string_literals;
    using namespace json;
    graph::Router<double> router(catalogue.GetGraph());
    Builder builder;
    builder.StartArray();
    for (const auto& command : document_.GetRoot().AsDict().at("stat_requests"s).AsArray()) {
        if (command.AsDict().at("type"s).AsString() == "Bus"s) {
            detail::AddBusInfo(catalogue, command.AsDict(), builder);
        }
        else if (command.AsDict().at("type"s).AsString() == "Stop"s) {
            detail::AddStopInfo(catalogue, command.AsDict(), builder);
        }
        else if (command.AsDict().at("type"s).AsString() == "Map"s) {
            detail::AddMapInfo(catalogue, map_renderer, command.AsDict(), builder);
        }
        else if (command.AsDict().at("type"s).AsString() == "Route"s) {
            detail::AddRouteInfo(catalogue, command.AsDict(), builder, router);
        }
    }
    builder.EndArray();
    Print(Document{ builder.Build() }, output);
}

void JsonReader::HandleRenderSettings(map_renderer::MapRenderer& map_render) {
    using namespace std::literals::string_literals;
    map_renderer::RenderSettings settings;
    const auto& settings_map = document_.GetRoot().AsDict().at("render_settings"s).AsDict();
    settings.width = settings_map.at("width"s).AsDouble();
    settings.height = settings_map.at("height"s).AsDouble();
    settings.padding = settings_map.at("padding"s).AsDouble();
    settings.line_width = settings_map.at("line_width"s).AsDouble();
    settings.stop_radius = settings_map.at("stop_radius"s).AsDouble();
    settings.bus_label_font_size = settings_map.at("bus_label_font_size"s).AsInt();
    settings.bus_label_offset = { settings_map.at("bus_label_offset"s).AsArray()[0].AsDouble(),
                                   settings_map.at("bus_label_offset"s).AsArray()[1].AsDouble() };
    settings.stop_label_font_size = settings_map.at("stop_label_font_size"s).AsInt();
    settings.stop_label_offset = { settings_map.at("stop_label_offset"s).AsArray()[0].AsDouble(),
                                    settings_map.at("stop_label_offset"s).AsArray()[1].AsDouble() };
    settings.underlayer_color = detail::GetColor(settings_map.at("underlayer_color"s));
    settings.underlayer_width = settings_map.at("underlayer_width"s).AsDouble();
    for (const auto& color: settings_map.at("color_palette"s).AsArray()) {
        settings.color_palette.push_back(detail::GetColor(color));
    }

    map_render.SetSettings(std::move(settings));
}

} // json_reader