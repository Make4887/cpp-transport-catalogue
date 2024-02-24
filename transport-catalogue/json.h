#pragma once

#include <cassert>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace json {

class Node;
// —охраните объ€влени€ Dict и Array без изменени€
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

// Ёта ошибка должна выбрасыватьс€ при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node {
public:
    using Value = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

    Node();
    Node(int value);
    Node(double value);
    Node(bool value);
    Node(std::string value);
    Node(std::nullptr_t value);
    Node(Array value);
    Node(Dict value);

    bool IsInt() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    [[nodiscard]] bool operator==(const Node& rhs) const noexcept;
    [[nodiscard]] bool operator!=(const Node& rhs) const noexcept;

    const Value& GetValue() const { return value_; }
private:
    Value value_;
};

class Document {
public:
    explicit Document(Node root);

    [[nodiscard]] bool operator==(const Document& rhs) const noexcept;
    [[nodiscard]] bool operator!=(const Document& rhs) const noexcept;

    const Node& GetRoot() const;

private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}  // namespace json