#include <windows.h>


typedef struct ClickerData {
    int interval;
    HWND hwnd;
} ClickerData;


#define ID_START 1
#define ID_STOP 2
#define ID_STATUS 3
#define ID_INTERVAL 4

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void init_clicker_thread(ClickerData *data);
void stop_clicking(HWND hwnd);
void start_clicking(const ClickerData *data);
int left_click();

BOOL get_pressed_key(const RAWINPUT *raw, const char key);