#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "drain.h"
#include "athreads.h"


#define ID_START 1
#define ID_STOP 2
#define ID_STATUS 3
#define ID_INTERVAL 4
#define G_KEY 0x47
#define ERASE_LINE "\033[2K"

int cur_threads = 0;
BOOL stop = FALSE;
int interval = 1000;


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {

    const wchar_t *class_name = L"Clicker";

    WNDCLASS wc = { 0 };

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = class_name;

    if (!RegisterClass(&wc)) {
        return 1;
    }

    // monitor res / 3
    int width  = GetSystemMetrics(SM_CXSCREEN) / 3;
    int height = GetSystemMetrics(SM_CYSCREEN) / 3;

    HWND window = CreateWindowEx(
        WS_EX_CLIENTEDGE, class_name, L"windwo yay", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height, // position and size
        NULL, NULL, hInstance, NULL
    );

    interval = _wtoi(pCmdLine);

    if (interval == 0) {
        interval = 1000;
    }

    if (!window) {
        return 1;
    }

    ShowWindow(window, nCmdShow);

    // raw input for hotkeys (toggle)
    RAWINPUTDEVICE rid = { 0 };
    rid.dwFlags = RIDEV_NOLEGACY | RIDEV_INPUTSINK;
    rid.hwndTarget = window;
    rid.usUsagePage = 1;
    rid.usUsage = 6; // keyboard usage class

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        return 1;
    }

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg) {

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT: {

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

            EndPaint(hwnd, NULL);

            return 0;
        }

        case WM_CREATE: {

            CreateWindowW(L"STATIC", L"Not Clicking",
                WS_VISIBLE | WS_CHILD | SS_CENTER,
                240, 50, 100, 25,
                hwnd, (HMENU)ID_STATUS, NULL,  NULL);

            // wchar_t *buffer = malloc(sizeof(L"Interval: ") + sizeof(int));
            // wsprintf(buffer, L"Interval: %d", interval);

            // CreateWindowW(L"STATIC", L"Interval: ",
            //     WS_VISIBLE | WS_CHILD | SS_CENTER,
            //     350, 50, 100, 25,
            //     hwnd, (HMENU)ID_INTERVAL, NULL,  NULL);

            // free(buffer);

            CreateWindowW(L"Button", L"Start clicking",
                WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                20, 50, 100, 25,
                hwnd, (HMENU)ID_START, NULL, NULL);

            CreateWindowW(L"Button", L"Stop clicking",
                WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                130, 50, 100, 25,
                hwnd, (HMENU)ID_STOP, NULL, NULL);

            break;
        }

        case WM_INPUT: {

            UINT dwSize;

            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
            LPBYTE lpb = malloc(sizeof(BYTE) * dwSize);
            if (lpb == NULL) {
                return 1;
            }

            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
                free(lpb);
                break;
            }

            RAWINPUT *raw = (RAWINPUT *)lpb;
            if (get_pressed_key(raw, 'g')) {
                stop_clicking(hwnd);
            }

            if (get_pressed_key(raw, 'r')) {
                start_clicking(&interval, hwnd);
            }

            free(lpb);
            break;
        }

        case WM_COMMAND: {

            if (LOWORD(wParam) == ID_START) {
                init_clicker_thread(&interval, hwnd);
            }

            if (LOWORD(wParam) == ID_STOP) {
                stop_clicking(hwnd);
            }

            break;
        }

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}


void init_clicker_thread(int *interval, HWND hwnd) {

    if (cur_threads == 0) { 

        stop = FALSE;

        A_Thread click_t = { 0 };
        click_t.function = start_clicking;
        click_t.args = &interval, hwnd; // TODO: make struct for args

        if (ath_create(&click_t) == 0) {
            cur_threads++;
        }

        else {
            MessageBox(hwnd, L"Could not create thread", L"Error", MB_OK);
        }

        // change status text to "Clicking"
        SetDlgItemText(hwnd, ID_STATUS, L"Clicking");
    }

    else {
        MessageBox(hwnd, L"Already clicking", L"Error", MB_OK);
    }
}



void stop_clicking(HWND hwnd) {

    if (cur_threads > 0) {
        stop = TRUE;
        SetDlgItemText(hwnd, ID_STATUS, L"Not clicking");
    }

    else {
        MessageBox(hwnd, L"Clicker Inactive", L"Error", MB_OK);
    }
}


void start_clicking(const int *interval_ms, HWND hwnd) {

    while (!stop) {

        if (left_click() != 0) {
            MessageBox(hwnd, L"Left click failed", L"Error", MB_OK);
            break;
        }

        Sleep(*interval_ms);
    }

    printf("exiting thread\n");
    cur_threads--;
    ExitThread(0);
}


int left_click() {

    INPUT inputs[2];

    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dx = 0;
    inputs[0].mi.dy = 0;

    // copy values
    inputs[0] = inputs[1];

    inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    UINT sent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));

    if (sent != ARRAYSIZE(inputs)) {
        return 1;
    }

    return 0;
}


BOOL get_pressed_key(RAWINPUT *raw, const char key) {
    
    if (raw->header.dwType == RIM_TYPEKEYBOARD &&
        raw->data.keyboard.Message == WM_KEYDOWN) {

        UINT pressed = MapVirtualKeyA(raw->data.keyboard.VKey, MAPVK_VK_TO_CHAR);
        if (pressed == key || pressed == toupper(key)) {
            return TRUE;
        }
    }

    return FALSE;
} 