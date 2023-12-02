#pragma once

#include "TextArea.hpp"
#include "LineNumDisplay.hpp"
#include "StatusBar.hpp"

#include <fstream>

class Editor {
    TextArea text_area;
    TextArea file_name_bar;
    LineNumDisplay line_num_display;
    StatusBar status_bar;

    TerminalIO& io = TerminalIO::get_instance();

    enum Status {
        EDITING,
        INPUTING_FILE_NAME
    } status = EDITING;

    bool should_quit = false;

public:
    Editor() :
        text_area(8, 1, 111, 28),
        line_num_display(0, 1, 8, 28),
        file_name_bar(0, 0, 119, 1),
        status_bar(0, 29, 119) {

        io.set_char_callback(
            [this](wchar_t ch) {
                text_area.process_char(ch);
                file_name_bar.process_char(ch);
            }
        );

        io.set_keydown_callback(
            [this](WORD vk_code, DWORD control_key_state) {
                if (control_key_state & LEFT_CTRL_PRESSED) {
                    if (vk_code == 'S') {
                        if (control_key_state & SHIFT_PRESSED) {
                            if (status == EDITING)
                                status = INPUTING_FILE_NAME;
                        }
                        else
                            save_to_file();
                    }

                    else if (vk_code == 'Q' || vk_code == 'W') {
                        should_quit = true;
                    }
                }

                switch (status) {
                case Editor::EDITING:
                    break;
                case Editor::INPUTING_FILE_NAME:
                    if (vk_code == VK_ESCAPE)
                        status = EDITING;
                    if (vk_code == 'S' && (control_key_state & LEFT_CTRL_PRESSED) && !(control_key_state & SHIFT_PRESSED))
                        status = EDITING;
                    break;
                default:
                    break;
                }

                text_area.process_keydown(vk_code, control_key_state);
                file_name_bar.process_keydown(vk_code, control_key_state);
            }
        );

        io.set_window_size_callback(
            [&](SHORT width, SHORT height) {
                text_area.set_width(width - 9);
                text_area.set_height(height - 2);
                line_num_display.set_height(height - 2);
                file_name_bar.set_width(width - 1);
                status_bar.set_width(width - 1);
                status_bar.set_top(height - 1);
            }
        );

        file_name_bar.set_background_color(COLOR::WHITE);
        file_name_bar.set_text_color(COLOR::CYAN);
        file_name_bar.set_selected_text_color(COLOR::CYAN);
        file_name_bar.set_wstring(L"Œﬁ±ÍÃ‚.txt");
    }

    void loop() {
        while (!should_quit) {
            switch (status) {
            case Editor::EDITING:
                text_area.set_active(true);
                file_name_bar.set_active(false);
                break;
            case Editor::INPUTING_FILE_NAME:
                text_area.set_active(false);
                file_name_bar.set_active(true);
                break;
            default:
                break;
            }

            text_area.render();

            line_num_display.first_line_num = text_area.get_first_line() + 1;
            line_num_display.current_line_num = text_area.get_current_line() + 1;
            line_num_display.last_line_num = text_area.get_line_count() + 1;
            line_num_display.render();

            file_name_bar.render();

            status_bar.line = text_area.get_current_line() + 1;
            status_bar.character = text_area.get_current_char() + 1;
            status_bar.render();

            io.render();
            Sleep(10);
        }
    }

    bool save_to_file() {
        std::ofstream fout(file_name_bar.get_wstring());
        if (fout.is_open())
            fout << text_area.get_utf_8_string();
        else return false;
        fout.close();
        return true;
    }

    bool save_to_temp_file() {
        HANDLE htemp = CreateFileW(
            (file_name_bar.get_wstring() + L".temp").c_str(),
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_HIDDEN,
            NULL
        );

        if (htemp == INVALID_HANDLE_VALUE)
            return false;

        auto content = text_area.get_utf_8_string();
        DWORD bytes_written;
        WriteFile(
            htemp,
            content.c_str(),
            content.length(),
            &bytes_written,
            NULL
        );
        return true;
    }

    bool read_from_temp_file(const std::wstring& file) {
        std::ifstream fin(file + L".temp");
        std::string str;
        if (!fin.is_open())
            return false;
        while (!fin.eof()) {
            std::string line;
            std::getline(fin, line);
            str += line + '\n';
        }
        fin.close();

        text_area.set_utf_8_string(str);
        file_name_bar.set_wstring(file);

        DeleteFileW((file + L".temp").c_str());

        return true;
    }

    bool read_from_file(const std::wstring& file) {
        std::ifstream fin(file);
        std::string str;
        if (!fin.is_open())
            return false; 
        while (!fin.eof()) {
            std::string line;
            std::getline(fin, line);
            str += line + '\n';
        }
        fin.close();

        text_area.set_utf_8_string(str);
        file_name_bar.set_wstring(file);

        return true;
    }

    ~Editor() {}
};

