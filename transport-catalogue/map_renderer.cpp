#include "map_renderer.h"

#include <string>
#include <utility>

namespace map_renderer {

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
    return {
        (coords.lng - min_lon_) * zoom_coeff_ + padding_,
        (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}

void MapRenderer::SetSettings(RenderSettings settings) {
    settings_ = std::move(settings);
}

std::set<std::string_view> MapRenderer::FilterBuses(const std::deque<Bus>& buses) const {
    std::set<std::string_view> result;
    for (const Bus& bus : buses) {
        if (!bus.route.empty()) {
            result.insert(bus.name);
        }
    }
    return result;
}

void MapRenderer::AddRoutSettings(svg::Polyline& rout, size_t index) const {
    using namespace std::literals::string_literals;
    rout.SetStrokeColor(settings_.color_palette[index % settings_.color_palette.size()])
        .SetFillColor("none"s)
        .SetFillColor("none"s)
        .SetStrokeWidth(settings_.line_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
}

void MapRenderer::AddCommonBusSettings(svg::Text& bus) const {
    using namespace std::literals::string_literals;
    bus.SetOffset(settings_.bus_label_offset)
       .SetFontSize(settings_.bus_label_font_size)
       .SetFontFamily("Verdana"s)
       .SetFontWeight("bold"s);
}

void MapRenderer::AddBusSettings(svg::Text& bus_name, size_t index) const {
    AddCommonBusSettings(bus_name);
    bus_name.SetFillColor(settings_.color_palette[index % settings_.color_palette.size()]);
}

void MapRenderer::AddBusBackingSettings(svg::Text& bus_name_backing) const {
    AddCommonBusSettings(bus_name_backing);
    bus_name_backing.SetFillColor(settings_.underlayer_color)
                    .SetStrokeColor(settings_.underlayer_color)
                    .SetStrokeWidth(settings_.underlayer_width)
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
}

void MapRenderer::AddStopSymbolSettings(svg::Circle& stop_symbol) const {
    using namespace std::literals::string_literals;
    stop_symbol.SetRadius(settings_.stop_radius);
    stop_symbol.SetFillColor("white"s);
}

void MapRenderer::AddCommonStopSettings(svg::Text& stop) const {
    using namespace std::literals::string_literals;
    stop.SetOffset(settings_.stop_label_offset);
    stop.SetFontSize(settings_.stop_label_font_size);
    stop.SetFontFamily("Verdana"s);
}

void MapRenderer::AddStopSettings(svg::Text& stop_name) const {
    using namespace std::literals::string_literals;
    AddCommonStopSettings(stop_name);
    stop_name.SetFillColor("black"s);
}

void MapRenderer::AddStopBackingSettings(svg::Text& stop_name_backing) const {
    using namespace std::literals::string_literals;
    AddCommonStopSettings(stop_name_backing);
    stop_name_backing.SetFillColor(settings_.underlayer_color);
    stop_name_backing.SetStrokeColor(settings_.underlayer_color);
    stop_name_backing.SetStrokeWidth(settings_.underlayer_width);
    stop_name_backing.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    stop_name_backing.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
}

} // map_renderer