#pragma once

#include "Terminal.hpp"

class Component {
    SHORT left;
    SHORT top;
    SHORT width;
    SHORT height;

protected:
    inline static TerminalIO& io = TerminalIO::get_instance();

public:
    Component(
        SHORT left, SHORT top,
        SHORT width, SHORT height
    ) :
        left(left), top(top), width(width), height(height) {}

public:
    virtual void render_relative(SHORT x, SHORT y) {
        left += x;
        top += y;
        render();
        left -= x;
        top -= y;
    };

    virtual void render() = 0;

    SHORT get_width() {
        return width;
    }
    SHORT get_height() {
        return height;
    }
    SHORT get_left() {
        return left;
    }
    SHORT get_top() {
        return top;
    }

    void set_width(SHORT width) {
        this->width = width;
    }
    void set_height(SHORT height) {
        this->height = height;
    }
    void set_left(SHORT left) {
        this->left = left;
    }
    void set_top(SHORT top) {
        this->top = top;
    }
};

