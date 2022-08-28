#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "drain.h"
#include "dthreads.h"

int cur_threads = 0;
BOOL stop = FALSE;
int interval = 100;


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {

    const wchar_t *class_name = L"Clicker";

    WNDCLASS wc = { 0 };

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = class_name;

    if (!RegisterClass(&wc)) {
        return 1;
    }

    // monitor res / 3
    int width  = GetSystemMetrics(SM_CXSCREEN) / 3;
    int height = GetSystemMetrics(SM_CYSCREEN) / 3;

    HWND window = CreateWindowEx(
        WS_EX_CLIENTEDGE, class_name, L"drain clicker :100:", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height, // position and size
        NULL, NULL, hInstance, NULL
    );

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

    ClickerData *data = malloc(sizeof(ClickerData));
    data->interval = interval;
    data->hwnd = hwnd;

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

            if (get_pressed_key(raw, 'r')) {
                if (cur_threads == 0) {
                    init_clicker_thread(data);
                }
                else {
                    stop_clicking(hwnd);
                }
            }

            free(lpb);
            break;
        }

        case WM_COMMAND: {

            if (LOWORD(wParam) == ID_START) {
                init_clicker_thread(data);
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


void init_clicker_thread(ClickerData *data) {

    if (cur_threads == 0) {

        stop = FALSE;

        DThread click_t = { 0 };
        click_t.function = start_clicking;
        click_t.args = data;

        if (dth_create(&click_t) == 0) {
            cur_threads++;
        }

        else {
            MessageBox(data->hwnd, L"Could not create thread", L"Error", MB_OK);
        }

        // change title text to "Clicking"
        SetDlgItemText(data->hwnd, ID_STATUS, L"Clicking");
    }

    else {
        MessageBox(data->hwnd, L"Already clicking", L"Error", MB_OK);
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


void start_clicking(const ClickerData *data) {

    while (1) {

        Sleep(data->interval);

        if (!stop) {
            left_click();
        }
        else {
            break;
        }
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
    return sent != ARRAYSIZE(inputs); // 0 if successful
}


BOOL get_pressed_key(const RAWINPUT *raw, const char key) {

    if (raw->header.dwType == RIM_TYPEKEYBOARD &&
        raw->data.keyboard.Message == WM_KEYDOWN) {

        UINT pressed = MapVirtualKeyA(raw->data.keyboard.VKey, MAPVK_VK_TO_CHAR);
        if (pressed == key || pressed == toupper(key)) {
            return TRUE;
        }
    }

    return FALSE;
} 