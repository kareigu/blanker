#include <assert.h>
#include <stdlib.h>
#include <wchar.h>

#define UNICODE
#define _UNICODE
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>

LPCWSTR CLASS_NAME = L"blanker";

#define S_COLOURS_SIZE 5
static COLORREF s_colours[S_COLOURS_SIZE] = {
    RGB(0, 0, 0),   RGB(255, 255, 255), RGB(255, 0, 0),
    RGB(0, 255, 0), RGB(0, 0, 255),
};
static size_t s_colour_index = 0;
static BOOL s_quit_on_focus_lost = TRUE;

static void set_cursor(BOOL on) {
    CURSORINFO inf;
    inf.cbSize = sizeof(CURSORINFO);
    GetCursorInfo(&inf);

    if (inf.flags == CURSOR_SHOWING && on == FALSE)
        ShowCursor(FALSE);
    else if (inf.flags == 0 && on == TRUE)
        ShowCursor(TRUE);
}

LRESULT CALLBACK wproc(HWND hwnd, UINT user_msg, WPARAM wparam, LPARAM lparam) {
    switch (user_msg) {
    case WM_CREATE: {
        return EXIT_SUCCESS;
    }
    case WM_CLOSE:
    case WM_DESTROY: {
        PostQuitMessage(EXIT_SUCCESS);
        return EXIT_SUCCESS;
    }
    case WM_ACTIVATEAPP: {
        if (s_quit_on_focus_lost && (BOOL)wparam == FALSE) {
            PostQuitMessage(EXIT_SUCCESS);
        } else if (!s_quit_on_focus_lost) {
            ShowCursor(!wparam);
        }
        return EXIT_SUCCESS;
    }
    case WM_PAINT: {
        PAINTSTRUCT painter;
        HDC hdc = BeginPaint(hwnd, &painter);

        FillRect(hdc, &painter.rcPaint,
                 CreateSolidBrush(s_colours[s_colour_index]));

        EndPaint(hwnd, &painter);
    }
    case WM_LBUTTONDOWN: {
        if (wparam & MK_CONTROL) {
            PostQuitMessage(EXIT_SUCCESS);
            return EXIT_SUCCESS;
        }
    }
    case WM_KEYDOWN: {
        switch (wparam) {
        case 'Q':
        case VK_ESCAPE: {
            PostQuitMessage(EXIT_SUCCESS);
            return EXIT_SUCCESS;
        }
        case VK_SPACE: {
            if (++s_colour_index >= S_COLOURS_SIZE)
                s_colour_index = 0;
            return !RedrawWindow(hwnd, NULL, NULL,
                                 RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
        }
        }
    }
    }

    return DefWindowProc(hwnd, user_msg, wparam, lparam);
}

static void parse_args(PWSTR args) {
    PWSTR flag_start = args;
    size_t flag_len = 0;
    BOOL in_flag = FALSE;
    for (size_t i = 0; args[i] != '\0';) {
        if (!in_flag && args[i] == '-' && args[i + 1] != '\0') {
            in_flag = TRUE;
            flag_start = &args[i + 1];
            flag_len = 0;
            i++;
            continue;
        }

        if (args[i] != ' ') {
            flag_len++;
            if (args[i + 1] != '\0')
                continue;
        }

        if (_wcsnicmp(flag_start, L"f", flag_len) == 0) {
            s_quit_on_focus_lost = FALSE;
        }

        in_flag = FALSE;
        i++;
    }
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR args,
                    int w_state) {
    parse_args(args);

    WNDCLASS wclass = {};
    wclass.lpfnWndProc = wproc;
    wclass.hInstance = instance;
    wclass.lpszClassName = CLASS_NAME;
    wclass.hIcon = (HICON)LoadImageW(instance, MAKEINTRESOURCE(1), IMAGE_ICON,
                                     256, 256, LR_DEFAULTCOLOR);

    RegisterClassW(&wclass);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, CLASS_NAME,
                               WS_POPUP | WS_VISIBLE | WS_MAXIMIZE,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                               CW_USEDEFAULT, NULL, NULL, instance, NULL);

    if (hwnd == NULL)
        return EXIT_FAILURE;

    set_cursor(FALSE);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CloseWindow(hwnd);
    UnregisterClass(CLASS_NAME, instance);

    return EXIT_SUCCESS;
}
