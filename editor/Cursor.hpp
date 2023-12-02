#pragma once

#include "Component.hpp"
#include <ctime>

class Cursor :
    public Component {
public:
    COLOR on_font_color = COLOR::WHITE;
    COLOR on_background_color = COLOR::LIGHT_BLUE;
    COLOR off_font_color = COLOR::BLACK;
    COLOR off_background_color = COLOR::WHITE;

    Cursor(
        SHORT left, SHORT top
    ) :
        Component(left, top, 1, 1) {}

private:
    clock_t last_show_t = clock();
    static constexpr clock_t PERIOD = 500;

    bool on = false;

public:
    void render() {
        auto t = clock();
        if (t - last_show_t > PERIOD) {
            last_show_t = t;
            on = !on;
        }
        TerminalIO::get_instance().set_font_style(
            get_left(), get_top(),
            on ? on_font_color : off_font_color,
            on ? on_background_color : off_background_color
        );
    }

    void should_be_on() {
        if (!on) {
            last_show_t += PERIOD;
            on = true;
        }
    }
};