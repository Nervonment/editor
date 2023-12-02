#pragma once

#include "Component.hpp"

#include <string>

class LineNumDisplay :
    public Component {
public:
    LineNumDisplay(
        SHORT left, SHORT top,
        SHORT width, SHORT height
    ) :
        Component(left, top, width, height) {}

public:
    size_t first_line_num = 0;
    size_t last_line_num = 0;
    size_t current_line_num = 0;
    
    COLOR background_color = COLOR::WHITE;
    COLOR font_color = COLOR::GRAY;
    COLOR current_line_font_color = COLOR::LIGHT_BLUE;

public:
    void render() {
        int i = 0;
        for (; i < last_line_num - first_line_num && i < get_height(); ++i) {
            auto line_num = std::to_string(i + first_line_num);
            line_num.insert(0, get_width() - line_num.size() - 2, ' ');
            line_num += "  ";
            io.draw_text_line(
                line_num.begin(),
                line_num.end(),
                get_left(),
                get_top() + i,
                get_width(),
                i + first_line_num == current_line_num ?
                current_line_font_color : font_color,
                background_color
            );
        }
        io.draw_rect(
            get_left(),
            get_top() + i,
            get_width(),
            get_height() - i,
            background_color
        );
    }
};

