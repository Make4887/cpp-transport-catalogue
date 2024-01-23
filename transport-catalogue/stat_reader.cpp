#include <iomanip>
#include <iostream>
#include <string>

#include "stat_reader.h"

namespace transport_catalogue {
namespace reader {
namespace detail{
void PrintBusInfo(const TransportCatalogue& transport_catalogue, std::string_view bus, std::ostream& output) {
    using namespace std::literals::string_literals;
    if (transport_catalogue.FindBus(bus)) {
        auto bus_info = transport_catalogue.GetBusInfo(bus);
        output << bus_info.count_all_stops << " stops on route, "s <<
            bus_info.count_unique_stops << " unique stops, "s <<
            bus_info.route_lenght << " route length, "s <<
            bus_info.curvature << " curvature"s << std::endl;
    }
    else {
        output << "not found"s << std::endl;
    }
}

void PrintStopInfo(const TransportCatalogue& transport_catalogue, std::string_view stop, std::ostream& output) {
    using namespace std::literals::string_literals;
    if (transport_catalogue.FindStop(stop)) {
        const std::set<std::string_view> buses = transport_catalogue.GetBusesPassingThroughStop(stop);
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

void ParseAndPrintStat(const TransportCatalogue& transport_catalogue, std::string_view request,
    std::ostream& output) {
    using namespace std::literals::string_literals;
    auto space = request.find(' ');
    const std::string command = std::string(request.substr(0, space));
    const std::string id = std::string(request.substr(space + 1));
    output << std::setprecision(6) << command << " "s << id << ": "s;
    if (command == "Bus"s) {
        detail::PrintBusInfo(transport_catalogue, id, output);
    }
    else if (command == "Stop"s) {
        detail::PrintStopInfo(transport_catalogue, id, output);
    }
}
}
}