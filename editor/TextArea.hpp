#pragma once

#include "Component.hpp"
#include "Cursor.hpp"

#include <vector>
#include <list>
#include <string>

class TextArea :
    public Component {
public:
    TextArea(
        SHORT left, SHORT top,
        SHORT width, SHORT height
    ) :
        Component(left, top, width, height),
        text({ Line() }),
        cursor_pos{
            0,0,
            text.begin()
        },
        vice_cursor_pos{
            0,0,
            text.begin()
        },
        first_line_it(text.begin()),
        cursor(left, top) {
        cursor.off_background_color = background_color;
        cursor.off_font_color = text_color;
    }

private:
    using Char = wchar_t;
    using Line = std::vector<Char>;
    using Text = std::list<Line>;
    struct CursorPos {
        size_t char_index;
        size_t line_index;

        Text::iterator line_it;

        // record the rightmost position of cursor
        // when moving up and downwards
        size_t          rightmost_cursor_pos = 0;

        bool operator>(const CursorPos& another) {
            if (line_index > another.line_index)
                return true;
            if (line_index < another.line_index)
                return false;
            return char_index > another.char_index;
        }
    };

    Text            text;
    CursorPos       cursor_pos;
    Cursor          cursor;

    bool is_selecting = false;
    CursorPos       vice_cursor_pos;

    size_t          first_line = 0;
    Text::iterator  first_line_it;
    int           horizontal_shift = 0;
    size_t get_first_char(Text::iterator it, bool* need_not_display = nullptr) {
        int width = 0;
        size_t first_char = 0;
        while (width < horizontal_shift && first_char < it->size())
            width += TerminalIO::get_font_width((*it)[first_char++]);
        if (width < horizontal_shift && need_not_display)
            *need_not_display = true;
        return first_char;
    }

    bool is_active = true;
public:
    void set_active(bool active){
        is_active = active;
    }

private:
    COLOR text_color = COLOR::BLACK;
    COLOR background_color = COLOR::WHITE;
    COLOR selected_text_color = COLOR::BLACK;
    COLOR selected_background_color = COLOR::LIGHT_GRAY;

public:
    void set_text_color(COLOR color) {
        text_color = color;
        cursor.off_font_color = color;
    }
    void set_background_color(COLOR color) {
        background_color = color;
        cursor.off_background_color = color;
    }
    void set_selected_text_color(COLOR color) {
        selected_text_color = color;
    }
    void set_selected_background_color(COLOR color) {
        selected_background_color = color;
    }

public:
    size_t get_line_count(){
        return text.size();
    }
    size_t get_first_line() {
        return first_line;
    }
    size_t get_current_line() {
        return
            is_selecting ?
            vice_cursor_pos.line_index :
            cursor_pos.line_index;
    }
    size_t get_current_char() {
        return
            is_selecting ?
            vice_cursor_pos.char_index :
            cursor_pos.char_index;
    }

private:
    void insert(Char ch) {
        auto& line = *cursor_pos.line_it;

        if (ch == '\r' || ch == '\n') {
            Line new_line(line.begin() + cursor_pos.char_index, line.end());
            line.erase(line.begin() + cursor_pos.char_index, line.end());
            ++cursor_pos.line_it;
            cursor_pos.line_it =
                text.insert(cursor_pos.line_it, std::move(new_line));
            ++cursor_pos.line_index;
            cursor_pos.char_index = 0;
        }
        else if (ch == '\t') {
            insert(' ');
            insert(' ');
            insert(' ');
            insert(' ');
        }
        else if (ch == '\b') {
            backspace();
        }
        else {
            line.insert(cursor_pos.line_it->begin() + cursor_pos.char_index, ch);
            ++cursor_pos.char_index;
        }

        check_cursor_pos(cursor_pos);
        cursor.should_be_on();
    }

    void backspace() {
        if (cursor_pos.char_index == 0) {
            if (cursor_pos.line_index > 0) {
                auto to_del = cursor_pos.line_it--;
                --cursor_pos.line_index;
                cursor_pos.char_index = cursor_pos.line_it->size();
                cursor_pos.line_it->insert(cursor_pos.line_it->end(), to_del->begin(), to_del->end());
                text.erase(to_del);
            }
        }
        else {
            cursor_pos.line_it->erase(cursor_pos.line_it->begin() + cursor_pos.char_index - 1);
            --cursor_pos.char_index;
        }

        check_cursor_pos(cursor_pos);
        cursor.should_be_on();
    }

    void delete_range(
        const CursorPos& first,
        const CursorPos& last
    ) {
        if (first.line_index == last.line_index) {
            first.line_it->erase(
                first.line_it->begin() + first.char_index, 
                first.line_it->begin() + last.char_index
            );
            cursor_pos.char_index = first.char_index;
        }
        else {
            last.line_it->erase(
                last.line_it->begin(),
                last.line_it->begin() + last.char_index
            );
            last.line_it->insert(
                last.line_it->begin(),
                first.line_it->begin(),
                first.line_it->begin() + first.char_index
            );
            text.erase(
                first.line_it, last.line_it
            );
            cursor_pos.line_it = last.line_it;
            cursor_pos.line_index = first.line_index;
            cursor_pos.char_index = first.char_index;
            cursor_pos.rightmost_cursor_pos = 0;
        }

        check_cursor_pos(cursor_pos);
    }

    void delete_selected() {
        delete_range(
            cursor_pos > vice_cursor_pos ?
            vice_cursor_pos : cursor_pos,
            cursor_pos > vice_cursor_pos ?
            cursor_pos : vice_cursor_pos
        );
    }

    std::wstring get_wstring_range(
        const CursorPos& first,
        const CursorPos& last
    ) {
        std::wstring wstr;
        if (first.line_index == last.line_index) {
            wstr.append(
                first.line_it->begin() + first.char_index,
                first.line_it->begin() + last.char_index
            );
        }
        else {
            wstr.append(
                first.line_it->begin() + first.char_index,
                first.line_it->end()
            );
            wstr += '\n';
            auto it = first.line_it;
            for (++it; it != last.line_it; ++it) {
                wstr.append(it->begin(), it->end());
                wstr += '\n';
            }
            wstr.append(
                last.line_it->begin(),
                last.line_it->begin() + last.char_index
            );
        }
        return wstr;
    }

    std::wstring get_selected() {
        return get_wstring_range(
            cursor_pos > vice_cursor_pos ?
            vice_cursor_pos : cursor_pos,
            cursor_pos > vice_cursor_pos ?
            cursor_pos : vice_cursor_pos
        );
    }

    void check_cursor_pos(CursorPos& cursor_pos) {
        if (cursor_pos.line_index >= first_line + get_height()) 
            first_line = cursor_pos.line_index - get_height() + 1;
        
        else if (cursor_pos.line_index < first_line) 
            first_line = cursor_pos.line_index;

        first_line_it = text.begin();
        for (size_t i = 0; i < first_line; ++i)
            ++first_line_it;

        int width = 0;
        auto first_char = get_first_char(cursor_pos.line_it);
        for (size_t i = first_char; i < cursor_pos.char_index; ++i)
            width += TerminalIO::get_font_width((*cursor_pos.line_it)[i]);
        if (width > get_width() - 1) {
            horizontal_shift += width - get_width() + 1;
            return;
        }

        width = 0;
        for (size_t i = 0; i < cursor_pos.char_index; ++i)
            width += TerminalIO::get_font_width((*cursor_pos.line_it)[i]);
        if (width < horizontal_shift)
            horizontal_shift = width;
    }

    void move_cursor_left(CursorPos& cursor_pos) {
        cursor_pos.rightmost_cursor_pos = 0;
        if (cursor_pos.char_index > 0) {
            --cursor_pos.char_index;
        }
        else if (cursor_pos.line_index > 0) {
            --cursor_pos.line_index;
            --cursor_pos.line_it;
            cursor_pos.char_index = cursor_pos.line_it->size();
        }
        cursor.should_be_on();
        check_cursor_pos(cursor_pos);
    }
    void move_cursor_right(CursorPos& cursor_pos) {
        cursor_pos.rightmost_cursor_pos = 0;
        if (cursor_pos.char_index < cursor_pos.line_it->size()) {
            ++cursor_pos.char_index;
        }
        else if (cursor_pos.line_index != text.size() - 1) {
            ++cursor_pos.line_index;
            ++cursor_pos.line_it;
            cursor_pos.char_index = 0;
        }
        cursor.should_be_on();
        check_cursor_pos(cursor_pos);
    }
    void move_cursor_up(CursorPos& cursor_pos) {
        if (cursor_pos.char_index > cursor_pos.rightmost_cursor_pos)
            cursor_pos.rightmost_cursor_pos = cursor_pos.char_index;
        if (cursor_pos.line_index > 0) {
            --cursor_pos.line_index;
            --cursor_pos.line_it;
            cursor_pos.char_index =
                cursor_pos.rightmost_cursor_pos < cursor_pos.line_it->size() ?
                cursor_pos.rightmost_cursor_pos : cursor_pos.line_it->size();
        }
        cursor.should_be_on();
        check_cursor_pos(cursor_pos);
    }
    void move_cursor_down(CursorPos& cursor_pos) {
        if (cursor_pos.char_index > cursor_pos.rightmost_cursor_pos)
            cursor_pos.rightmost_cursor_pos = cursor_pos.char_index;
        if (cursor_pos.line_index != text.size() - 1) {
            ++cursor_pos.line_index;
            ++cursor_pos.line_it;
            cursor_pos.char_index =
                cursor_pos.rightmost_cursor_pos < cursor_pos.line_it->size() ?
                cursor_pos.rightmost_cursor_pos : cursor_pos.line_it->size();
        }
        else {
            cursor_pos.line_index = text.size() - 1;
            cursor_pos.line_it = --text.end();
            cursor_pos.char_index = cursor_pos.line_it->size();
        }
        cursor.should_be_on();
        check_cursor_pos(cursor_pos);
    }

public:
    void render() {
        auto it = first_line_it;
        SHORT y = get_top();
        for (; y < get_top() + get_height() && it != text.end(); ++y, ++it) {
            auto first_char = get_first_char(it);
            if (it->end() - it->begin() >= long long(first_char))
                io.draw_text_line(
                    it->begin() + first_char,
                    it->end(),
                    get_left(), y,
                    get_width(),
                    text_color,
                    background_color
                );
        }
        io.draw_rect(
            get_left(), y,
            get_width(),
            get_top() + get_height() - y,
            background_color
        );

        if (is_selecting) {
            auto& right_cursor_pos =
                cursor_pos > vice_cursor_pos ?
                cursor_pos : vice_cursor_pos;
            auto& left_cursor_pos =
                cursor_pos > vice_cursor_pos ?
                vice_cursor_pos : cursor_pos;

            auto draw_selecting_text = [this](
                size_t line_index,
                Text::iterator line_it,
                size_t left_char,
                size_t right_char, 
                bool space_after_text = false
                ) {
                    bool need_not_display = false;
                    auto first_char = get_first_char(line_it, &need_not_display);
                    if (need_not_display) return;
                    
                    int width_before = 0;
                    int width = 0;
                    size_t i = first_char;
                    for (; i < left_char; ++i)
                        width_before += io.get_font_width((*line_it)[i]);
                    for (; i < right_char; ++i)
                        width += io.get_font_width((*line_it)[i]);
                    if (right_char >= first_char)
                        width++;
                    if (width > get_width() - width_before) width = get_width() - width_before;

                    if (left_char > first_char)
                        first_char = left_char;

                    if (right_char >= first_char)
                        io.draw_text_line(
                            line_it->begin() + first_char,
                            line_it->begin() + right_char,
                            get_left() + width_before,
                            get_top() + int(line_index) - int(first_line),
                            width,
                            selected_text_color,
                            selected_background_color,
                            space_after_text
                        );
            };

            if (left_cursor_pos.line_index == right_cursor_pos.line_index) {
                if (left_cursor_pos.line_index >= first_line &&
                    int(left_cursor_pos.line_index) - int(first_line) < get_height())
                    draw_selecting_text(
                        left_cursor_pos.line_index, left_cursor_pos.line_it,
                        left_cursor_pos.char_index, right_cursor_pos.char_index
                    );
            }
            else {
                if (left_cursor_pos.line_index >= first_line)
                    draw_selecting_text(
                        left_cursor_pos.line_index, left_cursor_pos.line_it,
                        left_cursor_pos.char_index, left_cursor_pos.line_it->size(),
                        true
                    );
                auto line_it = left_cursor_pos.line_it;
                ++line_it;
                for (size_t line_index = left_cursor_pos.line_index + 1;
                    line_index < right_cursor_pos.line_index && 
                    int(line_index) - int(first_line) < get_height(); ++line_index, ++line_it)
                    if (line_index >= first_line)
                        draw_selecting_text(
                            line_index, line_it,
                            get_first_char(line_it), line_it->size(),
                            true
                        );
                if (int(right_cursor_pos.line_index) - int(first_line) < get_height())
                    draw_selecting_text(
                        right_cursor_pos.line_index, right_cursor_pos.line_it,
                        get_first_char(right_cursor_pos.line_it), right_cursor_pos.char_index
                    );
            }
        }

        if (is_active && !is_selecting) {
            int rx = 0;
            for (size_t i = get_first_char(cursor_pos.line_it); i < cursor_pos.char_index; ++i)
                rx += TerminalIO::get_font_width((*cursor_pos.line_it)[i]);
            cursor.set_left(rx);
            cursor.set_top(int(cursor_pos.line_index) - int(first_line));
            cursor.render_relative(get_left(), get_top());
        }
    }

    void process_char(wchar_t ch) {
        if (is_active) {
            if (is_selecting) {
                delete_range(
                    cursor_pos > vice_cursor_pos ?
                    vice_cursor_pos : cursor_pos,
                    cursor_pos > vice_cursor_pos ?
                    cursor_pos : vice_cursor_pos
                );
                is_selecting = false;
            }
            insert(ch);
        }
    }

    void process_keydown(WORD vk_code, DWORD control_key_state) {
        if (is_active) {
            switch (vk_code) {
            // backspace
            case VK_BACK:
                if (!is_selecting)
                    backspace();
                else {
                    delete_selected();
                    is_selecting = false;
                }
                break;

            // move cursord
            case VK_LEFT:
                if (control_key_state & SHIFT_PRESSED) {
                    if (!is_selecting) {
                        is_selecting = true;
                        vice_cursor_pos = cursor_pos;
                    }
                    move_cursor_left(vice_cursor_pos);
                }
                else {
                    if (is_selecting) {
                        is_selecting = false;
                        cursor_pos =
                            cursor_pos > vice_cursor_pos ?
                            vice_cursor_pos : cursor_pos;
                        check_cursor_pos(cursor_pos);
                    }
                    else move_cursor_left(cursor_pos);
                }
                break;
            case VK_RIGHT:
                if (control_key_state & SHIFT_PRESSED) {
                    if (!is_selecting) {
                        is_selecting = true;
                        vice_cursor_pos = cursor_pos;
                    }
                    move_cursor_right(vice_cursor_pos);
                }
                else {
                    if (is_selecting) {
                        is_selecting = false;
                        cursor_pos =
                            cursor_pos > vice_cursor_pos ?
                            cursor_pos : vice_cursor_pos;
                        check_cursor_pos(cursor_pos);
                    }
                    else move_cursor_right(cursor_pos);
                }
                break;
            case VK_UP:
                if (control_key_state & SHIFT_PRESSED) {
                    if (!is_selecting) {
                        is_selecting = true;
                        vice_cursor_pos = cursor_pos;
                    }
                    move_cursor_up(vice_cursor_pos);
                }
                else {
                    if (is_selecting) {
                        is_selecting = false;
                        cursor_pos =
                            cursor_pos > vice_cursor_pos ?
                            vice_cursor_pos : cursor_pos;
                    }
                    move_cursor_up(cursor_pos);
                }
                break;
            case VK_DOWN:
                if (control_key_state & SHIFT_PRESSED) {
                    if (!is_selecting) {
                        is_selecting = true;
                        vice_cursor_pos = cursor_pos;
                    }
                    move_cursor_down(vice_cursor_pos);
                }
                else {
                    if (is_selecting) {
                        is_selecting = false;
                        cursor_pos =
                            cursor_pos > vice_cursor_pos ?
                            cursor_pos : vice_cursor_pos;
                    }
                    move_cursor_down(cursor_pos);
                }
                break;

            default:
                break;
            }

            // control shotcuts
            if(control_key_state & LEFT_CTRL_PRESSED)
                switch (vk_code) {
                case 'C':
                    if (is_selecting)
                        io.write_clipboard(
                            get_selected()
                        );
                    break;
                case 'X':
                    if (is_selecting) {
                        io.write_clipboard(
                            get_selected()
                        );
                        delete_selected();
                        is_selecting = false;
                    }
                    break;
                case 'A':
                    vice_cursor_pos.line_index = text.size() - 1;
                    vice_cursor_pos.line_it = --text.end();
                    vice_cursor_pos.char_index = vice_cursor_pos.line_it->size();

                    cursor_pos.line_index = 0;
                    cursor_pos.line_it = text.begin();
                    cursor_pos.char_index = 0;

                    check_cursor_pos(vice_cursor_pos);
                    
                    is_selecting = true;
                    break;
                default:
                    break;
                }
        }
    }


    void set_wstring(std::wstring wstr) {
        text.clear();

        size_t i = 0, j = 0;
        while (j < wstr.size()) {
            i = j;
            while (wstr[j] != '\n' && j < wstr.size())++j;
            text.push_back(Line(wstr.begin() + i, wstr.begin() + j));
        }

        if (text.empty())
            text.push_back(Line());
        cursor_pos = {
            0,0,text.begin()
        };
        first_line = 0;
        first_line_it = text.begin();
        horizontal_shift = 0;
    }

    void set_utf_8_string(std::string str) {
           text.clear();

           int rest_byte_count = 0;
           Char current_char = 0;

           size_t i = 0, j = 0;
           while (j < str.size()) {
               i = j;
               while (str[j] != '\n' && j < str.size())++j;
               Line vline;
               vline.reserve((j - i) / 3);
               while (i < j) {
                   auto ch = str[i];
                   if (rest_byte_count == 0) {
                       if ((ch & 0b1000'0000) == 0) {
                           rest_byte_count = 0;
                           current_char = ch & 0b0111'1111;
                       }
                       else if ((ch & 0b1110'0000) == 0b1100'0000) {
                           rest_byte_count = 1;
                           current_char = ch & 0b0001'1111;
                       }
                       else if ((ch & 0b1111'0000) == 0b1110'0000) {
                           rest_byte_count = 2;
                           current_char = ch & 0b0000'1111;
                       }
                       else if ((ch & 0b1111'1000) == 0b1111'0000) {
                           rest_byte_count = 3;
                           current_char = ch & 0b0000'0111;
                       }
                   }
                   else {
                       current_char <<= 6;
                       current_char |= (ch & 0b0011'1111);
                       --rest_byte_count;
                   }

                   if (rest_byte_count == 0)
                       vline.push_back(current_char);

                   ++i;
               }
               text.insert(text.end(), std::move(vline));
               ++j;
           }
           if (text.empty())
               text.push_back(Line());
           cursor_pos = {
               0,0,text.begin()
           };
           first_line = 0;
           first_line_it = text.begin();
           horizontal_shift = 0;
    }

    std::wstring get_wstring() {
        std::wstring wstr;
        for (auto it = text.begin(); it != text.end();) {
            wstr.append(it->begin(), it->end());
            if (++it != text.end())
                wstr += '\n';
        }
        return wstr;
    }

    std::string get_utf_8_string() {
        std::string str;
        for (auto it = text.begin(); it != text.end(); ) {
            std::string line;
            line.reserve(it->size());
            for (auto ch : *it) {
                if (ch < 0x007f)
                    line.push_back(ch);
                else if (ch < 0x07ff) {
                    line.push_back(0b11000000 | ((ch >> 6) & 0b00011111));
                    line.push_back(0b10000000 | (ch & 0b00111111));
                }
                else if (ch < 0xffff) {
                    line.push_back(0b11100000 | ((ch >> 12) & 0b00001111));
                    line.push_back(0b10000000 | ((ch >> 6) & 0b00111111));
                    line.push_back(0b10000000 | (ch & 0b00111111));
                }
            }
            str += line;
            if (++it != text.end())
                str += '\n';
        }

        return str;
    }
};

