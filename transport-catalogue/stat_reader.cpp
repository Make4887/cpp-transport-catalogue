#include <iomanip>
#include <iostream>
#include <string>

#include "stat_reader.h"

namespace transportcatalogue {
namespace reader {

void ParseAndPrintStat(const TransportCatalogue& transport_catalogue, std::string_view request,
    std::ostream& output) {
    using namespace std::literals::string_literals;
    auto space = request.find(' ');
    const std::string command = std::string(request.substr(0, space));
    const std::string id = std::string(request.substr(space + 1));
    output << std::setprecision(6) << command << " "s << id << ": "s;
    if (command == "Bus"s) {
        if (transport_catalogue.FindBus(id)) {
            output << transport_catalogue.CountBusStops(id) << " stops on route, "s <<
                transport_catalogue.CountUniqueBusStops(id) << " unique stops, "s <<
                transport_catalogue.ComputeRouteLength(id) << " route length"s << std::endl;
        }
        else {
            output << "not found"s << std::endl;
        }
    }
    else if (command == "Stop"s) {
        if (transport_catalogue.FindStop(id)) {
            const std::set<std::string_view> buses = transport_catalogue.GetBusesPassingThroughStop(id);
            if (buses.empty()) {
                output << "no buses"s << std::endl;
            }
            else {
                output << "buses"s;
                for (auto bus : buses) {
                    std::string x = std::string(bus);
                    output << " "s << x;
                }
                output << std::endl;
            }
        }
        else {
            output << "not found"s << std::endl;
        }
    }
}
}
}