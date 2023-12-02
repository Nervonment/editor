#include "Editor.hpp"
#include <string>

Editor editor;

BOOL handle_exit(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_CLOSE_EVENT)
        editor.save_to_temp_file();
    return true;
}

std::wstring ansi_to_unicode(const char* src_str) {
    auto len = MultiByteToWideChar(CP_ACP, 0, src_str, -1, NULL, 0);
    wchar_t* w_cstr = new wchar_t[len + 1];
    memset(w_cstr, 0, len + 1);
    MultiByteToWideChar(CP_ACP, 0, src_str, -1, w_cstr, len);
    std::wstring wstr = w_cstr;
    delete[] w_cstr;
    return wstr;
}

int main(int argc, char* argv[]) {
    SetConsoleCtrlHandler(
        handle_exit, true
    );
    if (argc == 2) {
        auto file_name_unicode = ansi_to_unicode(argv[1]);
        if (!editor.read_from_temp_file(file_name_unicode))
            if (!editor.read_from_file(file_name_unicode))
                return -1;
    }

    editor.loop();
}