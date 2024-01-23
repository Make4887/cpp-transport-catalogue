#include <algorithm>
#include <cassert>
#include <iterator>

#include "input_reader.h"

namespace transport_catalogue {
namespace reader {
namespace parse {
/**
 * Парсит строку вида "10.123,  -30.1837" и возвращает пару координат (широта, долгота)
 */
transport_catalogue::geo::Coordinates Coordinates(std::string_view str) {
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');

    if (comma == str.npos) {
        return { nan, nan };
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);

    auto comma2 = str.find(',', not_space2);

    double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
    double lng;
    if (comma2 == str.npos) {
        lng = std::stod(std::string(str.substr(not_space2)));
    }
    else {
        lng = std::stod(std::string(str.substr(not_space2, comma2 - not_space2)));
    }
    
    return { lat, lng };
}

std::unordered_map<std::string_view, int> Distances(std::string_view str) {
    std::unordered_map<std::string_view, int> result;
    auto comma = str.find(',');
    comma = str.find(',', comma + 1);

    while (comma != str.npos) {
        auto not_space = str.find_first_not_of(' ', comma + 1);
        auto leter_m = str.find('m', not_space);
        int dist = std::stoi(std::string(str.substr(not_space, leter_m - not_space)));
        auto leter_o = str.find('o', leter_m);
        auto not_space2 = str.find_first_not_of(' ', leter_o + 1);
        comma = str.find(',', not_space2);
        std::string_view stop_name;
        if (comma == str.npos) {
            stop_name = str.substr(not_space2);
        }
        else {
            stop_name = str.substr(not_space2, comma - not_space2);
        }
        result.insert({ stop_name, dist });
    }
    return result;
}

namespace detail{
/**
 * Удаляет пробелы в начале и конце строки
 */
std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

/**
 * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
 */
std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}
}

/**
 * Парсит маршрут.
 * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
 * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
 */
std::vector<std::string_view> Route(std::string_view route) {
    if (route.find('>') != route.npos) {
        return detail::Split(route, '>');
    }

    auto stops = detail::Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}

reader::CommandDescription CommandDescription(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    return { std::string(line.substr(0, space_pos)),
            std::string(line.substr(not_space, colon_pos - not_space)),
            std::string(line.substr(colon_pos + 1)) };
}

}

void InputReader::ParseLine(std::string_view line) {
    auto command_description = parse::CommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

void InputReader::ApplyCommands([[maybe_unused]] TransportCatalogue& catalogue) const {
    using namespace std::literals::string_literals;
    for (const CommandDescription& command_description : commands_) {
        if (command_description.command == "Stop"s) {
            catalogue.AddStop(command_description.id, parse::Coordinates(command_description.description));
        }
    }
    for (const CommandDescription& command_description : commands_) {
        if (command_description.command == "Stop"s) {
            for (auto [neighbour_name, dist] : parse::Distances(command_description.description)) {
                catalogue.AddDistances(command_description.id, neighbour_name, dist);
            }
        }
    }
    for (const CommandDescription& command_description : commands_) {
        if (command_description.command == "Bus"s) {
            catalogue.AddBus(command_description.id, parse::Route(command_description.description));
        }
    }
}
}
}