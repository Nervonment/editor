#pragma once

#include <Windows.h>
#include <thread>
#include <functional>

class InputListener {
    HANDLE hstdin;
    bool _failed_to_init = false;

    static constexpr size_t INPUT_BUFFER_SIZE = 8;
    INPUT_RECORD input_buffer[INPUT_BUFFER_SIZE]{};

    std::thread listen_thread;

public:
    using KEYDOWN_CALLBACK = std::function<void(WORD vk_code, DWORD control_key_state)>;
    using WINDOW_SIZE_CALLBACK = std::function<void(SHORT width, SHORT height)>;
    using CHAR_CALLBACK = std::function<void(wchar_t ch)>;

private:
    KEYDOWN_CALLBACK keydown_callback = nullptr;
    WINDOW_SIZE_CALLBACK window_size_callback = nullptr;
    CHAR_CALLBACK char_callback = nullptr;

public:
    InputListener() :
        hstdin(GetStdHandle(STD_INPUT_HANDLE)),
        listen_thread(&InputListener::listen, this) {
        if (!SetConsoleMode(hstdin, ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS))
            _failed_to_init = true;
        listen_thread.detach();
    }

    bool failed_to_init() {
        return hstdin != INVALID_HANDLE_VALUE || _failed_to_init;
    }

    void set_keydown_callback(KEYDOWN_CALLBACK callback) {
        keydown_callback = callback;
    }
    void set_window_size_callback(WINDOW_SIZE_CALLBACK callback) {
        window_size_callback = callback;
    }
    void set_char_callback(CHAR_CALLBACK callback) {
        char_callback = callback;
    }

private:
    void listen() {
        while (true) {
            DWORD events_read;
            ReadConsoleInput(
                hstdin,
                input_buffer,
                INPUT_BUFFER_SIZE,
                &events_read
            );

            for (int i = 0; i < events_read; ++i) {
                auto& event = input_buffer[i];
                switch (event.EventType) {
                case KEY_EVENT:
                    if (event.Event.KeyEvent.bKeyDown) {
                        if (keydown_callback)
                            keydown_callback(
                                event.Event.KeyEvent.wVirtualKeyCode,
                                event.Event.KeyEvent.dwControlKeyState
                            );
                        if (char_callback
                            && (event.Event.KeyEvent.uChar.UnicodeChar > 31 ||
                                event.Event.KeyEvent.uChar.UnicodeChar == '\r' ||
                                event.Event.KeyEvent.uChar.UnicodeChar == '\t'))
                            char_callback(event.Event.KeyEvent.uChar.UnicodeChar);
                    }
                    break;
                case WINDOW_BUFFER_SIZE_EVENT:
                    if (window_size_callback)
                        window_size_callback(
                            event.Event.WindowBufferSizeEvent.dwSize.X,
                            event.Event.WindowBufferSizeEvent.dwSize.Y
                        );
                    break;
                default:
                    break;
                }
            }
        }
    }
};

