#pragma once

#include "InputListener.hpp"
#include "OutputWriter.hpp"
#include <string>

class TerminalIO:
    public InputListener,
    public OutputWriter {
    TerminalIO() :
        InputListener(),
        OutputWriter() {}
public:
    static TerminalIO& get_instance() {
        static TerminalIO instance;
        return instance;
    }
    void set_window_size_callback(WINDOW_SIZE_CALLBACK callback) {
        auto window_size = get_window_size();
        callback(window_size.X, window_size.Y);
        InputListener::set_window_size_callback(callback);
    }

    bool write_clipboard(const std::wstring& text){
        if (!OpenClipboard(NULL))
            return false;
        if (!EmptyClipboard()) {
            CloseClipboard();
            return false;
        }

        HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE, (text.length() + 1) * sizeof(wchar_t));
        if (!hmem) {
            CloseClipboard();
            return false;
        }

        wchar_t* data = (wchar_t*)GlobalLock(hmem);
        if (!data) {
            CloseClipboard();
            GlobalFree(hmem);
            return false;
        }
        memcpy(
            data,
            text.c_str(),
            (text.length() + 1) * sizeof(wchar_t)
        );
        GlobalUnlock(hmem);
        
        bool res = SetClipboardData(CF_UNICODETEXT, hmem);
        CloseClipboard();
        GlobalFree(hmem);
        return res;
    }
};

