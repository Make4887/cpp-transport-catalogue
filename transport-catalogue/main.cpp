#include <iostream>

#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"

using namespace std;
using namespace transport_catalogue;

int main() {
    TransportCatalogue catalogue;
    json_reader::JsonReader reader(cin);
    reader.ApplyBaseCommands(catalogue);

    map_renderer::MapRenderer renderer;
    reader.HandleRenderSettings(renderer);
    reader.ApplyStatCommands(catalogue, renderer, cout);
}