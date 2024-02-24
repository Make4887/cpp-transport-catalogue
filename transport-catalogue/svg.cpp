#include "svg.h"

namespace svg {

using namespace std::literals;

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}
struct ColorPrinter {
    std::ostream& out;

    void operator()(std::monostate) {
        using namespace std::literals;
        out << "none"sv;
    }

    void operator()(std::string color) {
        out << color;
    }

    void operator()(Rgb rgb) {
        using namespace std::literals;
        out << "rgb("sv << static_cast<int>(rgb.red) << ',' << static_cast<int>(rgb.green) << ',' << static_cast<int>(rgb.blue) << ')';
    }

    void operator()(Rgba rgba) {
        using namespace std::literals;
        out << "rgba("sv << static_cast<int>(rgba.red) << ',' << static_cast<int>(rgba.green) << ',' << static_cast<int>(rgba.blue) << ',' << rgba.opacity << ')';
    }
};

std::ostream& operator<<(std::ostream& out, const Color& color) {
    std::visit(ColorPrinter{ out }, color);
    return out;
}

std::ostream& operator<<(std::ostream& out, const StrokeLineCap& stoke_line_cap) {
    switch (stoke_line_cap) {
    case StrokeLineCap::BUTT:
        out << "butt"sv;
        break;
    case StrokeLineCap::ROUND:
        out << "round"sv;
        break;
    case StrokeLineCap::SQUARE:
        out << "square"sv;
        break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& stroke_line_join) {
    switch (stroke_line_join) {
    case StrokeLineJoin::ARCS:
        out << "arcs"sv;
        break;
    case StrokeLineJoin::BEVEL:
        out << "bevel"sv;
        break;
    case StrokeLineJoin::MITER:
        out << "miter"sv;
        break;
    case StrokeLineJoin::MITER_CLIP:
        out << "miter-clip"sv;
        break;
    case StrokeLineJoin::ROUND:
        out << "round"sv;
        break;
    }
    return out;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center) {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius) {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\" "sv;
    // Выводим атрибуты, унаследованные от PathProps
    RenderAttrs(context.out);
    out << " />"sv;
}

Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    bool first = true;
    for (Point point : points_) {
        if (!first) {
            out << " "sv;
        }
        else {
            first = false;
        }
        out << point.x << ","sv << point.y;
    }
    out << "\""sv;
    RenderAttrs(context.out);
    out << " />"sv;
}

Text& Text::SetPosition(Point pos) {
    pos_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = font_family;
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = font_weight;
    return *this;
}

Text& Text::SetData(std::string data) {
    data_ = data;
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text"sv;
    RenderAttrs(context.out);
    out << " x = \""sv << pos_.x << "\" y=\""sv << pos_.y << "\" "sv;
    out << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv;
    out << "font-size=\""sv << size_ << "\" "sv;
    if (!font_family_.empty()) {
        out << "font-family=\""sv << font_family_ << "\" "sv;
    }
    if (!font_weight_.empty()) {
        out << "font-weight=\""sv << font_weight_ << "\" "sv;
    }
    out << ">"sv;

    for (char c : data_) {
        switch (c) {
        case '"':
            out << "&quot;"sv;
            break;
        case '<':
            out << "&lt;"sv;
            break;
        case '>':
            out << "&gt;"sv;
            break;
        case '&':
            out << "&amp;"sv;
            break;
        case '\'':
            out << "&apos;"sv;
            break;
        default:
            out.put(c);
        }
    }
    out << "</text>";
}

// Добавляет в svg-документ объект-наследник svg::Object
void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.push_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
    RenderContext context(out, 2, 2);
    for (auto& object : objects_) {
        object->Render(context);
    }
    out << "</svg>"sv;
}

}  // namespace svg