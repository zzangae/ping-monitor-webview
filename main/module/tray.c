/**
 * Tray Icon Module Implementation
 */

#include "tray.h"
#include <shellapi.h>

void InitTrayIcon(HWND hwnd)
{
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = hwnd;
    g_nid.uID = ID_TRAY_ICON;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy(g_nid.szTip, L"Ping Monitor");

    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void RemoveTrayIcon(void)
{
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

void ShowTrayMenu(HWND hwnd)
{
    HMENU hMenu = CreatePopupMenu();

    AppendMenuW(hMenu, MF_STRING, ID_TRAY_START, g_isRunning ? L"일시정지" : L"시작");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_BROWSER, L"브라우저 열기");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    UINT checkFlag = g_notifSettings.enabled ? MF_CHECKED : MF_UNCHECKED;
    AppendMenuW(hMenu, MF_STRING | checkFlag, ID_TRAY_NOTIFICATIONS, L"알림 활성화");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_RELOAD_CONFIG, L"설정 불러오기");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_CHANGE_PORT, L"포트 변경...");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"종료");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}