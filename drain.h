#include <windows.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int left_click();

void init_clicker_thread(int *interval, HWND hwnd);
void start_clicking(const int *interval_ms, HWND hwnd);
void stop_clicking(HWND hwnd);

BOOL get_pressed_key(RAWINPUT *raw, const char key);
