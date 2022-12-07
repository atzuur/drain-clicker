#ifndef _WIN32
#error Windows 32/64 only
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

static HANDLE lmbEvent = NULL;
static int avgDelay = 0;
static WCHAR toggleKey = L'x';
static BOOL toggled = FALSE;

LRESULT WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

    switch (message) {
        case WM_INPUT: {
            UINT dwSize;

            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
            LPBYTE lpb = malloc(sizeof(BYTE) * dwSize);
            if (lpb == NULL)
                return 0;

            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize,
                                sizeof(RAWINPUTHEADER)) != dwSize) {
                free(lpb);
                break;
            }

            RAWINPUT* raw = (RAWINPUT*)lpb;

            if (raw->header.dwType == RIM_TYPEKEYBOARD &&
                raw->data.keyboard.Message == WM_KEYDOWN) {

                UINT key = MapVirtualKeyA(raw->data.keyboard.VKey, MAPVK_VK_TO_CHAR);

                if (key == toggleKey || towlower(key) == towlower(toggleKey)) {
                    toggled = !toggled;
                    if (!toggled)
                        ResetEvent(lmbEvent);
                }

            } else if (raw->header.dwType == RIM_TYPEMOUSE && toggled) {
                if (raw->data.mouse.usButtonFlags == RI_MOUSE_LEFT_BUTTON_DOWN) {
                    SetEvent(lmbEvent);
                } else if (raw->data.mouse.usButtonFlags == RI_MOUSE_LEFT_BUTTON_UP) {
                    ResetEvent(lmbEvent);
                }
            }

            free(lpb);
            break;
        }
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}

void Clicker() {

    BOOL shouldDouble = FALSE;
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;

    while (1) {
        WaitForSingleObject(lmbEvent, INFINITE);

        shouldDouble = (rand() % 10) > 1; // 80% chance of double click
        if (shouldDouble) {
            Sleep(rand() % 4 + 3); // 3-7 ms
            SendInput(1, &input, sizeof(INPUT));
        }

        SendInput(1, &input, sizeof(INPUT));
        Sleep(avgDelay + (rand() % avgDelay / 4));
    }
}

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR pCmdLine, int nCmdShow) {

    srand(time(NULL));

    WNDCLASSA wclass = {0};
    wclass.hInstance = hInstance;
    wclass.lpfnWndProc = WndProc;
    wclass.lpszClassName = "dc";

    if (!RegisterClassA(&wclass))
        return 1;

    HWND hwnd = CreateWindowA(wclass.lpszClassName, NULL, 0, 0, 0, 0, 0, 0, NULL, hInstance, NULL);
    if (!hwnd)
        return 1;

    RAWINPUTDEVICE rid[2] = {0};
    rid[0].dwFlags = RIDEV_NOLEGACY | RIDEV_INPUTSINK;
    rid[0].hwndTarget = hwnd;
    rid[0].usUsagePage = 1;
    rid[0].usUsage = 2;

    rid[1].dwFlags = RIDEV_NOLEGACY | RIDEV_INPUTSINK;
    rid[1].hwndTarget = hwnd;
    rid[1].usUsagePage = 1;
    rid[1].usUsage = 6;

    if (!RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE)))
        return 1;

    int argc;
    PWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc < 2) {
        MessageBoxW(hwnd, L"Usage: draincl.exe <avg delay> <toggle key (default X)>",
                    L"drain clicker", MB_OK);
        return 1;
    }

    avgDelay = _wtoi(argv[1]);
    if (argc >= 3)
        toggleKey = argv[2][0];

    lmbEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    HANDLE clickThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Clicker, NULL, 0, NULL);

    MSG msg = {0};

    while (GetMessage(&msg, hwnd, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CloseHandle(lmbEvent);
    CloseHandle(clickThread);

    return 0;
}
