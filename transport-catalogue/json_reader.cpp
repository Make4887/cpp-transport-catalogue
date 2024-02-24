#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

json::Node AddBusInfo(const transport_catalogue::TransportCatalogue& catalogue, const json::Dict& command) {
    using namespace std::literals::string_literals;
    using namespace json;
    if (catalogue.FindBus(command.at("name"s).AsString())) {
        auto bus_info = catalogue.GetBusInfo(command.at("name"s).AsString());
        return Dict{
            {"curvature"s, Node(bus_info.curvature)},
            {"request_id"s, Node(command.at("id"s).AsInt())},
            {"route_length"s, Node(bus_info.route_lenght)},
            {"stop_count"s, Node(static_cast<int>(bus_info.count_all_stops))},
            {"unique_stop_count"s, Node(static_cast<int>(bus_info.count_unique_stops))}
        };
    }

    return Dict{
        {"request_id"s, Node(command.at("id"s).AsInt())},
        {"error_message"s, Node("not found"s)}
    };
}

json::Node AddStopInfo(const transport_catalogue::TransportCatalogue& catalogue, const json::Dict& command) {
    using namespace std::literals::string_literals;
    using namespace json;
    if (catalogue.FindStop(command.at("name"s).AsString())) {
        const std::set<std::string_view> buses = catalogue.GetBusesPassingThroughStop(command.at("name"s).AsString());
        Array ar;
        for (auto bus : buses) {
            ar.push_back(Node(std::string(bus)));
        }
        return Dict{
            {"buses"s, Node(std::move(ar))},
            {"request_id"s, Node(command.at("id"s).AsInt())}
        };
    }
    return Dict{
        {"request_id"s, Node(command.at("id"s).AsInt())},
        {"error_message"s, Node("not found"s)}
    };
}

json::Node AddMapInfo(const transport_catalogue::TransportCatalogue& catalogue, 
                        const map_renderer::MapRenderer& map_renderer, const json::Dict& command) {
    using namespace std::literals::string_literals;
    using namespace json;
    RequestHandler request_handler(catalogue, map_renderer);
    std::ostringstream buf_stream;
    request_handler.RenderMap(buf_stream);
    return Dict{
            {"map"s, Node(buf_stream.str())},
            {"request_id"s, Node(command.at("id"s).AsInt())}
    };
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
    for (const auto& command : document_.GetRoot().AsMap().at("base_requests"s).AsArray()) {
        const auto com = command.AsMap();

        if (com.at("type"s).AsString() == "Stop"s) {
            catalogue.AddStop(com.at("name"s).AsString(), geo::Coordinates{ com.at("latitude"s).AsDouble(), com.at("longitude"s).AsDouble() });
        }
    }

    for (const auto& command : document_.GetRoot().AsMap().at("base_requests"s).AsArray()) {
        const auto com = command.AsMap();

        if (com.at("type"s).AsString() == "Stop"s) {
            for (auto [neighbour_name, dist] : com.at("road_distances"s).AsMap()) {
                catalogue.AddDistances(com.at("name"s).AsString(), neighbour_name, dist.AsInt());
            }
        }
    }

    for (const auto& command : document_.GetRoot().AsMap().at("base_requests"s).AsArray()) {
        const auto com = command.AsMap();

        if (com.at("type"s).AsString() == "Bus"s) {
            catalogue.AddBus(com.at("name"s).AsString(), detail::Route(com), com.at("is_roundtrip"s).AsBool());
        }
    }
}

void JsonReader::ApplyStatCommands(const transport_catalogue::TransportCatalogue& catalogue, 
                                    const map_renderer::MapRenderer& map_renderer, std::ostream& output) const {
    using namespace std::literals::string_literals;
    using namespace json;
    Array res_array;

    for (const auto& command : document_.GetRoot().AsMap().at("stat_requests"s).AsArray()) {
        if (command.AsMap().at("type"s).AsString() == "Bus"s) {
            res_array.push_back(detail::AddBusInfo(catalogue, command.AsMap()));
        }
        else if (command.AsMap().at("type"s).AsString() == "Stop"s) {
            res_array.push_back(detail::AddStopInfo(catalogue, command.AsMap()));
        }
        else if (command.AsMap().at("type"s).AsString() == "Map"s) {
            res_array.push_back(detail::AddMapInfo(catalogue, map_renderer, command.AsMap()));
        }
    }
    json::Document result(Node( std::move(res_array) ));
    Print(result, output);
}

void JsonReader::HandleRenderSettings(map_renderer::MapRenderer& map_render) {
    using namespace std::literals::string_literals;
    map_renderer::RenderSettings settings;
    const auto& settings_map = document_.GetRoot().AsMap().at("render_settings"s).AsMap();
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