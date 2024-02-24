#pragma once
#include <sstream>

#include "json.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

namespace json_reader {

class JsonReader {
public:
    JsonReader(std::istream& input);

    void ApplyBaseCommands(transport_catalogue::TransportCatalogue& catalogue) const;
    void ApplyStatCommands(const transport_catalogue::TransportCatalogue& catalogue, const map_renderer::MapRenderer& map_renderer,
                            std::ostream& output) const;
    void HandleRenderSettings(map_renderer::MapRenderer& map_render);

private:
    json::Document document_;
};

}