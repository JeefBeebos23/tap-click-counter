#include <windows.h>
#include <stdio.h>

#define ID_INCREMENT 1001
#define ID_DECREMENT 1002

static HWND g_hStatic = NULL;
static int g_counter = 0;

static void UpdateCounterText(void) {
    char buf[16];
    sprintf(buf, "%d", g_counter);
    SetWindowTextA(g_hStatic, buf);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HFONT hFont = CreateFontA(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");

        g_hStatic = CreateWindowA("STATIC", "0",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            0, 20, 300, 80, hwnd, NULL, NULL, NULL);
        SendMessageA(g_hStatic, WM_SETFONT, (WPARAM)hFont, TRUE);

        CreateWindowA("BUTTON", "-",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            40, 140, 100, 50, hwnd, (HMENU)ID_DECREMENT, NULL, NULL);

        CreateWindowA("BUTTON", "+",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            160, 140, 100, 50, hwnd, (HMENU)ID_INCREMENT, NULL, NULL);

        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_INCREMENT:
            g_counter++;
            UpdateCounterText();
            break;
        case ID_DECREMENT:
            if (g_counter > 0) g_counter--;
            UpdateCounterText();
            break;
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "TapCounterWindowClass";

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(0, CLASS_NAME, "Tap Counter",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 320, 260,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {0};
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}
