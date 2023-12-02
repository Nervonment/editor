#pragma once

#include <Windows.h>

enum class COLOR {
    BLACK = 0,
    RED = FOREGROUND_RED,
    GREEN = FOREGROUND_GREEN,
    BLUE = FOREGROUND_BLUE,
    MAGENTA = FOREGROUND_RED | FOREGROUND_BLUE,
    YELLOW = FOREGROUND_RED | FOREGROUND_GREEN,
    CYAN = FOREGROUND_BLUE | FOREGROUND_GREEN,
    LIGHT_GRAY = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    
    GRAY = BLACK | FOREGROUND_INTENSITY,
    LIGHT_RED = RED | FOREGROUND_INTENSITY,
    LIGHT_GREEN = GREEN | FOREGROUND_INTENSITY,
    LIGHT_BLUE = BLUE | FOREGROUND_INTENSITY,
    LIGHT_MAGENTA = MAGENTA | FOREGROUND_INTENSITY,
    LIGHT_YELLOW = YELLOW | FOREGROUND_INTENSITY,
    LIGHT_CYAN = CYAN | FOREGROUND_INTENSITY,
    WHITE = LIGHT_GRAY | FOREGROUND_INTENSITY
};

class OutputWriter {
    HANDLE      hstdout;
    CHAR_INFO*  buffer = nullptr;
    SHORT       window_width = 0;
    SHORT       window_height = 0;
    WORD        background_color = BACKGROUND_INTENSITY;

public:
    OutputWriter() :
        hstdout(GetStdHandle(STD_OUTPUT_HANDLE)) {
        check_window_size();

        // hide the cursor
        CONSOLE_CURSOR_INFO cursor_info;
        GetConsoleCursorInfo(hstdout, &cursor_info);
        cursor_info.bVisible = false;
        SetConsoleCursorInfo(hstdout, &cursor_info);
    }

    COORD get_window_size() {
        return { window_width, window_height };
    }

private:
    SHORT get_x(SHORT screen_x, SHORT y) {
        SHORT x1 = 0;
        for (SHORT width = 0; width < screen_x;) {
            size_t i = y * window_width + x1++;
            width += get_font_width(buffer[i].Char.UnicodeChar);
        }
        return x1;
    }

private:
    struct RectArgs {
        SHORT left;
        SHORT top;
        SHORT width;
        SHORT height;
        COLOR background_color;
        bool clear_the_text = true;
    };
    std::vector<RectArgs> rects{};
    size_t rect_cnt = 0;
public:
    // delayed draw
    // the rectangles must be drawn after all texts were drawn
    void draw_rect(
        SHORT left,
        SHORT top,
        SHORT width,
        SHORT height,
        COLOR background_color,
        bool clear_the_text = true
    ) {
        if (rects.size() > rect_cnt) {
            rects[rect_cnt] = {
               left,top,width,height,
               background_color,clear_the_text
            };
        }
        else {
            rects.push_back(
                {
                    left,top,width,height,
                    background_color,clear_the_text
                }
            );
        }
        ++rect_cnt;
    }

private:
    // called in render()
    void draw_rect_impl() {
        rect_cnt = 0;
        for (auto& rect_args : rects) {
            auto y1 = rect_args.top;
            auto y2 = y1 + rect_args.height;
            for (size_t y = y1; y < y2; ++y) {
                auto x1 = get_x(rect_args.left, y);
                auto x2 = x1 + rect_args.width;
                if (x2 > window_width || y2 > window_height)
                    return;
                for (size_t x = x1; x < x2; ++x) {
                    size_t i = y * window_width + x;
                    if (i > window_height * window_width)
                        return;
                    buffer[i].Attributes &= 0xff0f;
                    buffer[i].Attributes |= (static_cast<WORD>(rect_args.background_color) << 4);
                    if (rect_args.clear_the_text)
                        buffer[i].Char.UnicodeChar = ' ';
                }
            }
        }
        return;
    }

public:
    template <typename Iterator>
    void draw_text_line(
        Iterator first,
        Iterator last,
        SHORT x,
        SHORT y,
        SHORT width,
        COLOR color,
        COLOR background_color,
        bool space_after_text = true
    ) {
        SHORT written_width = 0;
        size_t i = y * window_width + get_x(x, y);
        size_t i0 = i;
        auto it = first;
        if (it != last)
            written_width += get_font_width(*it);
        for (; it != last && written_width <= width && i < window_height * window_width;) {
            buffer[i].Attributes &= 0xff00;
            buffer[i].Attributes |= static_cast<WORD>(color);
            buffer[i].Attributes |= (static_cast<WORD>(background_color) << 4);
            buffer[i].Char.UnicodeChar = *it;
            ++it, ++i;
            if (it != last)
                written_width += get_font_width(*it);
        }
        if (it != last)
            written_width -= get_font_width(*it);
        if (space_after_text)
            while (written_width < width && i < window_height * window_width) {
                buffer[i].Attributes &= 0xff0f;
                buffer[i].Attributes |= (static_cast<WORD>(background_color) << 4);
                buffer[i].Char.UnicodeChar = ' ';
                ++i; ++written_width;
            }
    }

    bool set_font_style(
        SHORT x,
        SHORT y,
        COLOR color,
        COLOR background_color
    ) {
        size_t i = y * window_width + get_x(x, y);
        if (i > window_height * window_width)
            return false;
        buffer[i].Attributes &= 0xff00;
        buffer[i].Attributes |= static_cast<WORD>(color);
        buffer[i].Attributes |= (static_cast<WORD>(background_color) << 4);
    }

public:
    static SHORT get_font_width(wchar_t ch) {
        if (
            // 中日韩汉字部首补充
            // 康熙部首
            // 表意文字描述字符
            // 中日韩符号和标点符号
            // 日文平假名
            // 日文片假名
            // 注音
            // 韩文兼容字母
            // 汉文训读
            // 注音符号扩展
            // 中日韩汉语笔画
            // 片假名扩展
            // 中日韩带圈字符及月份
            // 中日韩兼容
            // 中日韩兼容表意文字扩展A
            // 易经64卦符号
            // 中日韩统一表意文字
            (0x2e80 <= ch && ch <= 0x9fff) ||

            // 韩文音节
            // 韩文字母扩展
            (0xac00 <= ch && ch <= 0xd7ff) ||

            // 中日韩兼容表意文字
            (0xf900 <= ch && ch <= 0xfaff) ||

            // 竖排形式符号
            (0xfe10 <= ch && ch <= 0xfe1f) ||

            // 中日韩兼容形式符号
            // 小型变体形式符号
            (0xfe30 <= ch && ch <= 0xfe6f) ||

            // 全角符号
            (0xff00 <= ch && ch <= 0xff60) ||

            // 各种符号、emoji
            (0x1f300 <= ch && ch <= 0x1faff) ||

            // 中日韩统一表意文字扩展B~F、增补
            (0x20000 <= ch && ch <= 0x2fa1f)
            )
            return 2;

        return 1;
    }

public:
    void render() {
        draw_rect_impl();

        for (SHORT y = 0; y < window_height; ++y) {
            size_t i = y * window_width;
            size_t j = i + window_width - 1;
            while (j > i)
                if (get_font_width(buffer[i++].Char.UnicodeChar) == 2)
                    buffer[j--].Char.UnicodeChar = 0;
        }

        SMALL_RECT rect{ 0,0,window_width - 1, window_height - 1 };
        WriteConsoleOutput(
            hstdout,
            buffer,
            { window_width, window_height },
            { 0,0 },
            &rect
        );

        flush();
        check_window_size();
    }

private:
    void flush() {
        for (size_t i = 0; i < window_width * window_height; ++i) {
            buffer[i].Attributes &= 0xff0f;
            buffer[i].Attributes |= (static_cast<WORD>(background_color) << 4);
            buffer[i].Char.UnicodeChar = ' ';
        }
    }

    void check_window_size() {
        CONSOLE_SCREEN_BUFFER_INFO csbi_info;
        GetConsoleScreenBufferInfo(hstdout, &csbi_info);
        SHORT width = csbi_info.srWindow.Right - csbi_info.srWindow.Left + 1;
        SHORT height = csbi_info.srWindow.Bottom - csbi_info.srWindow.Top + 1;
        if (width != window_width || height != window_height) {
            SetConsoleScreenBufferSize(
                hstdout, { width, height }
            );
            set_window_size(width, height);
        }
    }

    void set_window_size(SHORT window_width, SHORT window_height) {
        this->window_width = window_width;
        this->window_height = window_height;
        if (buffer)
            delete[]buffer;
        buffer = new CHAR_INFO[window_height * (window_width)]{};
        flush();
    }

public:
    ~OutputWriter() {
        delete[]buffer;
    }
};

