/**
 * Port Management Module Implementation
 */

#include "port.h"
#include "../http_server.h"
#include "../browser_monitor.h"
#include <winsock2.h>
#include <shlwapi.h>
#include <stdio.h>

BOOL IsPortAvailable(int port)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        return FALSE;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(port);

    int result = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    closesocket(sock);

    return (result == 0);
}

int FindAvailablePort(int startPort, int endPort)
{
    for (int port = startPort; port <= endPort; port++)
    {
        if (IsPortAvailable(port))
        {
            return port;
        }
    }
    return -1;
}

BOOL RestartServer(int newPort)
{
    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    StopHttpServer();
    Sleep(500);

    if (StartHttpServer(newPort, exeDir))
    {
        g_currentPort = newPort;
        wprintf(L"서버 포트 변경: %d\n", newPort);
        return TRUE;
    }

    return FALSE;
}

LRESULT CALLBACK PortDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int *pNewPort = NULL;
    static HWND hEdit = NULL;
    static BOOL *pResult = NULL;

    switch (message)
    {
    case WM_CREATE:
    {
        CREATESTRUCT *pCreate = (CREATESTRUCT *)lParam;

        void **params = (void **)pCreate->lpCreateParams;
        pNewPort = (int *)params[0];
        pResult = (BOOL *)params[1];

        int availablePort = FindAvailablePort(HTTP_PORT_MIN, HTTP_PORT_MAX);

        wchar_t infoText[512];
        if (availablePort != -1)
        {
            swprintf(infoText, 512,
                     L"현재 포트: %d\n\n"
                     L"권장 포트: %d (사용 가능)\n\n"
                     L"새 포트 번호를 입력하세요\n"
                     L"(범위: %d-%d)",
                     g_currentPort, availablePort, HTTP_PORT_MIN, HTTP_PORT_MAX);
        }
        else
        {
            swprintf(infoText, 512,
                     L"현재 포트: %d\n\n"
                     L"경고: 사용 가능한 포트를 찾을 수 없습니다\n\n"
                     L"새 포트 번호를 입력하세요\n"
                     L"(범위: %d-%d)",
                     g_currentPort, HTTP_PORT_MIN, HTTP_PORT_MAX);
        }

        HWND hStatic = CreateWindowW(
            L"STATIC", infoText,
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            20, 20, 340, 100,
            hWnd, NULL, GetModuleHandle(NULL), NULL);

        wchar_t portText[16];
        swprintf(portText, 16, L"%d", availablePort != -1 ? availablePort : g_currentPort);

        hEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT", portText,
            WS_VISIBLE | WS_CHILD | ES_NUMBER | ES_CENTER | WS_TABSTOP,
            20, 130, 120, 30,
            hWnd, (HMENU)1002, GetModuleHandle(NULL), NULL);

        CreateWindowW(
            L"BUTTON", L"확인",
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP,
            160, 130, 100, 30,
            hWnd, (HMENU)IDOK, GetModuleHandle(NULL), NULL);

        CreateWindowW(
            L"BUTTON", L"취소",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP,
            270, 130, 90, 30,
            hWnd, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);

        HFONT hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Malgun Gothic");
        SendMessage(hStatic, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(GetDlgItem(hWnd, IDOK), WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(GetDlgItem(hWnd, IDCANCEL), WM_SETFONT, (WPARAM)hFont, TRUE);

        SetFocus(hEdit);
        SendMessage(hEdit, EM_SETSEL, 0, -1);

        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            wchar_t buffer[16];
            GetWindowTextW(hEdit, buffer, 16);
            int newPort = _wtoi(buffer);

            if (newPort >= HTTP_PORT_MIN && newPort <= HTTP_PORT_MAX)
            {
                if (IsPortAvailable(newPort) || newPort == g_currentPort)
                {
                    *pNewPort = newPort;
                    *pResult = TRUE;
                    DestroyWindow(hWnd);
                }
                else
                {
                    wchar_t errorMsg[256];
                    swprintf(errorMsg, 256,
                             L"포트 %d는 이미 사용 중입니다.\n\n다른 포트를 입력하세요.",
                             newPort);
                    MessageBoxW(hWnd, errorMsg, L"포트 사용 중", MB_OK | MB_ICONWARNING);
                }
            }
            else
            {
                wchar_t errorMsg[256];
                swprintf(errorMsg, 256,
                         L"포트는 %d부터 %d 사이여야 합니다.",
                         HTTP_PORT_MIN, HTTP_PORT_MAX);
                MessageBoxW(hWnd, errorMsg, L"입력 오류", MB_OK | MB_ICONWARNING);
            }
            return 0;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            *pResult = FALSE;
            DestroyWindow(hWnd);
            return 0;
        }
        break;

    case WM_CLOSE:
        *pResult = FALSE;
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, message, wParam, lParam);
}

void ChangeServerPort(HWND hwnd)
{
    static BOOL classRegistered = FALSE;
    if (!classRegistered)
    {
        WNDCLASSW wc = {0};
        wc.lpfnWndProc = PortDialogProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"PortChangeDialog";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);

        if (RegisterClassW(&wc))
        {
            classRegistered = TRUE;
        }
    }

    int newPort = 0;
    BOOL dialogResult = FALSE;
    void *params[2] = {&newPort, &dialogResult};

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"PortChangeDialog",
        L"포트 변경",
        WS_VISIBLE | WS_SYSMENU | WS_CAPTION | DS_MODALFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 380, 230,
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        params);

    if (!hDlg)
    {
        MessageBoxW(hwnd, L"다이얼로그 생성 실패", L"오류", MB_OK | MB_ICONERROR);
        return;
    }

    RECT rcDlg, rcOwner;
    GetWindowRect(hDlg, &rcDlg);
    GetWindowRect(GetDesktopWindow(), &rcOwner);
    int x = (rcOwner.right - (rcDlg.right - rcDlg.left)) / 2;
    int y = (rcOwner.bottom - (rcDlg.bottom - rcDlg.top)) / 2;
    SetWindowPos(hDlg, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);

    EnableWindow(hwnd, FALSE);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_QUIT)
        {
            break;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    EnableWindow(hwnd, TRUE);
    SetForegroundWindow(hwnd);

    if (dialogResult && newPort > 0 && newPort != g_currentPort)
    {
        if (RestartServer(newPort))
        {
            wchar_t successMsg[256];
            swprintf(successMsg, 256,
                     L"포트가 %d로 변경되었습니다.\n\n"
                     L"새 주소: http://localhost:%d",
                     newPort, newPort);
            MessageBoxW(hwnd, successMsg, L"포트 변경 완료", MB_OK | MB_ICONINFORMATION);

            wchar_t url[256];
            swprintf(url, 256, L"http://localhost:%d/web/graph.html", newPort);
            OpenBrowser(url);
        }
        else
        {
            MessageBoxW(hwnd, L"포트 변경에 실패했습니다.", L"오류", MB_OK | MB_ICONERROR);
        }
    }
}