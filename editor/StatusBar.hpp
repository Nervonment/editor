#pragma once

#include "Component.hpp"

#include <string>

class StatusBar :
    public Component {
public:
    StatusBar(
        SHORT left, SHORT top,
        SHORT width
    ) :
        Component(left, top, width, 1) {}

public:
    int line = 1;
    int character = 1;

    COLOR font_color = COLOR::WHITE;
    COLOR background_color = COLOR::CYAN;

    void render() {
        std::wstring position = L"  ÐÐ      ÁÐ    ";
        auto col = std::to_wstring(line);
        auto row = std::to_wstring(character);
        position.replace(4, col.size(), col.c_str());
        position.replace(11, row.size(), row.c_str());
        io.draw_text_line(
            position.begin(),
            position.end(),
            get_left(), get_top(),
            get_width(),
            font_color,
            background_color
        );
    }
};

