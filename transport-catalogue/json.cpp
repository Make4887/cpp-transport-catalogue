#include "json.h"

using namespace std;

namespace json {

namespace {

Node LoadNode(std::istream& input);

Node LoadArray(std::istream& input) {
    Array result;
    char c;
    for (; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }
    if (!input) {
        throw ParsingError("Array parsing error");
    }
    return Node(std::move(result));
}

Node LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
        };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
        };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    }
    else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return std::stoi(parsed_num);
            }
            catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return std::stod(parsed_num);
    }
    catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует использовать после считывания открывающего символа ":
Node LoadString(std::istream& input) {
    using namespace std::literals;

    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        }
        else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
            case 'n':
                s.push_back('\n');
                break;
            case 't':
                s.push_back('\t');
                break;
            case 'r':
                s.push_back('\r');
                break;
            case '"':
                s.push_back('"');
                break;
            case '\\':
                s.push_back('\\');
                break;
            default:
                // Встретили неизвестную escape-последовательность
                throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        }
        else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        }
        else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return Node(std::move(s));
}

Node LoadDict(std::istream& input) {
    Dict result;
    for (char c; input >> c && c != '}';) {
        if (c == '"') {
            std::string key = LoadString(input).AsString();
            if (input >> c && c == ':') {
                if (result.find(key) != result.end()) {
                    throw ParsingError("Duplicate key '"s + key + "' have been found");
                }
                result.emplace(std::move(key), LoadNode(input));
            }
            else {
                throw ParsingError(": is expected but '"s + c + "' has been found"s);
            }
        }
        else if (c != ',') {
            throw ParsingError(R"(',' is expected but ')"s + c + "' has been found"s);
        }
    }
    if (!input) {
        throw ParsingError("Dictionary parsing error"s);
    }
    return Node(std::move(result));
}

Node LoadNull(std::istream& input) {
    using namespace std::literals;
    for (const char c : "ull"s) {
        if (c != input.get()) {
            throw ParsingError("Null parsing error"s);
        }
    }
    if (std::isalpha(input.peek())) {
        throw ParsingError("Parsing error"s);
    }
    return {};
}

Node LoadBool(std::istream& input) {
    using namespace std::literals;
    if (input.peek() == 't') {
        for (const char c : "true"s) {
            if (c != input.get()) {
                throw ParsingError("Bool parsing error"s);
            }
        }
        if (std::isalpha(input.peek())) {
            throw ParsingError("Parsing error"s);
        }
        return true;
    }
    if (input.peek() == 'f') {
        for (const char c : "false"s) {
            if (c != input.get()) {
                throw ParsingError("Bool parsing error"s);
            }
        }
        if (std::isalpha(input.peek())) {
            throw ParsingError("Parsing error"s);
        }
        return false;
    }
    throw ParsingError("Not Bool"s);
}

Node LoadNode(std::istream& input) {
    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input);
    }
    else if (c == '{') {
        return LoadDict(input);
    }
    else if (c == '"') {
        return LoadString(input);
    }
    else if (c == 'n') {
        return LoadNull(input);
    }
    else if (c == 't' || c == 'f') {
        input.putback(c);
        return LoadBool(input);
    }
    else {
        input.putback(c);
        return LoadNumber(input);
    }
}

}  // namespace

Node::Node() = default;

Node::Node(int value)
    : value_(value)
{
}

Node::Node(double value)
    : value_(value)
{
}

Node::Node(bool value)
    : value_(value)
{
}

Node::Node(std::string value)
    : value_(std::move(value))
{
}

Node::Node(std::nullptr_t value)
    : value_(value)
{
}

Node::Node(Array value)
    : value_(std::move(value))
{
}

Node::Node(Dict value)
    : value_(std::move(value))
{
}

bool Node::IsInt() const {
    return holds_alternative<int>(value_);
}
bool Node::IsDouble() const {
    return holds_alternative<int>(value_) || holds_alternative<double>(value_);
}
bool Node::IsPureDouble() const {
    return holds_alternative<double>(value_);
}
bool Node::IsBool() const {
    return holds_alternative<bool>(value_);
}
bool Node::IsString() const {
    return holds_alternative<std::string>(value_);
}
bool Node::IsNull() const {
    return holds_alternative<std::nullptr_t>(value_);
}
bool Node::IsArray() const {
    return holds_alternative<Array>(value_);
}
bool Node::IsMap() const {
    return holds_alternative<Dict>(value_);
}

int Node::AsInt() const {
    if (IsInt()) {
        return std::get<int>(value_);
    }

    throw std::logic_error("Wrong type");
}
bool Node::AsBool() const {
    if (IsBool()) {
        return std::get<bool>(value_);
    }

    throw std::logic_error("Wrong type");
}
double Node::AsDouble() const {
    if (IsPureDouble()) {
        return std::get<double>(value_);
    }

    if (IsInt()) {
        return std::get<int>(value_);
    }

    throw std::logic_error("Wrong type");
}
const std::string& Node::AsString() const {
    if (IsString()) {
        return std::get<std::string>(value_);
    }

    throw std::logic_error("Wrong type");
}
const Array& Node::AsArray() const {
    if (IsArray()) {
        return std::get<Array>(value_);
    }

    throw std::logic_error("Wrong type");
}
const Dict& Node::AsMap() const {
    if (IsMap()) {
        return std::get<Dict>(value_);
    }

    throw std::logic_error("Wrong type");
}

[[nodiscard]] bool Node::operator==(const Node& rhs) const noexcept {
    return value_ == rhs.value_;
}

[[nodiscard]] bool Node::operator!=(const Node& rhs) const noexcept {
    return !(*this == rhs);
}

Document::Document(Node root)
    : root_(move(root)) {
}

[[nodiscard]] bool Document::operator==(const Document& rhs) const noexcept {
    return root_ == rhs.root_;
}

[[nodiscard]] bool Document::operator!=(const Document& rhs) const noexcept {
    return !(*this == rhs);
}

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(istream& input) {
    return Document{ LoadNode(input) };
}

// Контекст вывода, хранит ссылку на поток вывода и текущий отсуп
struct PrintContext {
    PrintContext(std::ostream& out)
        : out(out) {
    }

    PrintContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent) {
    }

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    // Возвращает новый контекст вывода с увеличенным смещением
    PrintContext Indented() const {
        return { out, indent_step, indent_step + indent };
    }

    std::ostream& out;
    int indent_step = 4;
    int indent = 0;
};

void PrintNode(const Node& node, const PrintContext& ctx);

// Шаблон, подходящий для вывода double и int
template <typename Value>
void PrintValue(const Value& value, const PrintContext& ctx) {
    auto& out = ctx.out;
    out << value;
}

// Перегрузка функции PrintValue для вывода значений null
void PrintValue(std::nullptr_t, const PrintContext& ctx) {
    auto& out = ctx.out;
    out << "null"sv;
}

// Другие перегрузки функции PrintValue пишутся аналогично
void PrintValue(const std::string& value, const PrintContext& ctx) {
    auto& out = ctx.out;
    out << '"';
    for (char ch : value) {
        // Встретили начало escape-последовательности
        // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
        switch (ch) {
        case '\n':
            out << '\\' << 'n';
            break;
        case '\t':
            out << '\\' << 't';
            break;
        case '\r':
            out << '\\' << 'r';
            break;
        case '"':
            out << '\\' << '"';
            break;
        case '\\':
            out << '\\' << '\\';
            break;
        default:
            out << ch;
        }
    }
    out << '"';
}

void PrintValue(const Array& values, const PrintContext& ctx) {
    auto& out = ctx.out;
    out << '[';
    bool first = true;
    for (const auto& value : values) {
        if (first) {
            first = false;
        }
        else {
            out << ',';
        }
        PrintNode(value, ctx);
    }
    out << ']';
}

void PrintValue(const Dict& values, const PrintContext& ctx) {
    auto& out = ctx.out;
    bool first = true;
    out << '{';
    for (const auto& [key, value] : values) {
        if (first) {
            first = false;
        }
        else {
            out << ',';
        }
        PrintValue(key, ctx);
        out << ':';
        PrintNode(value, ctx);
    }
    out << '}';
}

void PrintValue(bool value, const PrintContext& ctx) {
    auto& out = ctx.out;
    if (value) {
        out << "true"sv;
    }
    else {
        out << "false"sv;
    }
}

void PrintNode(const Node& node, const PrintContext& ctx) {
    std::visit(
        [&ctx](const auto& value) { PrintValue(value, ctx); },
        node.GetValue());
}

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), output);
}

}  // namespace json