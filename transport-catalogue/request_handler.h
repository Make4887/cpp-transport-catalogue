#pragma once
#include <sstream>
#include <string_view>

#include "map_renderer.h"
#include "transport_catalogue.h"
 
 class RequestHandler {
 public:
     // MapRenderer ����������� � ��������� ����� ��������� �������
     RequestHandler(const transport_catalogue::TransportCatalogue& db, const map_renderer::MapRenderer& renderer);

     // ���� ����� ����� ����� � ��������� ����� ��������� �������
     void RenderMap(std::ostream& output) const;

 private:
     // RequestHandler ���������� ��������� �������� "������������ ����������" � "������������ �����"
     const transport_catalogue::TransportCatalogue& db_;
     const map_renderer::MapRenderer& renderer_;

     void AddBus(svg::Document& doc, svg::Point screen_coord, std::string_view bus, size_t index) const;
     void AddStop(svg::Document& doc, svg::Point screen_coord, std::string_view stop) const;
 };