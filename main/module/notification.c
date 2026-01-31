/**
 * Notification Module Implementation - Custom Notification Window
 */

#include "notification.h"
#include <shellapi.h>
#include <shlwapi.h>
#include <stdio.h>
#include <time.h>

#define MAX_NOTIFICATIONS 5
static NotificationWindow g_notifications[MAX_NOTIFICATIONS] = {0};
static BOOL g_notificationSystemInitialized = FALSE;
static WNDCLASSW g_notificationClass = {0};
static HFONT g_titleFont = NULL;
static HFONT g_messageFont = NULL;

LRESULT CALLBACK NotificationWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void InitNotificationSystem(void)
{
    if (g_notificationSystemInitialized)
        return;

    g_titleFont = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    g_messageFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    g_notificationClass.lpfnWndProc = NotificationWindowProc;
    g_notificationClass.hInstance = GetModuleHandle(NULL);
    g_notificationClass.lpszClassName = L"PingMonitorNotification";
    g_notificationClass.hbrBackground = NULL;
    g_notificationClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    g_notificationClass.style = 0;

    if (RegisterClassW(&g_notificationClass))
    {
        g_notificationSystemInitialized = TRUE;
    }
}

void CleanupNotificationSystem(void)
{
    for (int i = 0; i < MAX_NOTIFICATIONS; i++)
    {
        if (g_notifications[i].hwnd)
        {
            if (IsWindow(g_notifications[i].hwnd))
            {
                DestroyWindow(g_notifications[i].hwnd);
            }
            g_notifications[i].hwnd = NULL;
        }
    }

    if (g_titleFont)
    {
        DeleteObject(g_titleFont);
        g_titleFont = NULL;
    }
    if (g_messageFont)
    {
        DeleteObject(g_messageFont);
        g_messageFont = NULL;
    }
}

int FindFreeNotificationSlot(void)
{
    for (int i = 0; i < MAX_NOTIFICATIONS; i++)
    {
        if (!g_notifications[i].hwnd || !IsWindow(g_notifications[i].hwnd))
        {
            return i;
        }
    }
    return -1;
}

void RepositionNotifications(void)
{
    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

    int yOffset = 10;
    for (int i = 0; i < MAX_NOTIFICATIONS; i++)
    {
        if (g_notifications[i].hwnd && IsWindow(g_notifications[i].hwnd))
        {
            int x = workArea.right - NOTIFICATION_WIDTH - 10;
            int y = workArea.bottom - NOTIFICATION_HEIGHT - yOffset;

            SetWindowPos(g_notifications[i].hwnd, HWND_TOPMOST, x, y, 0, 0,
                         SWP_NOSIZE | SWP_NOACTIVATE);

            yOffset += NOTIFICATION_HEIGHT + 10;
        }
    }
}

LRESULT CALLBACK NotificationWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        SetTimer(hwnd, 1, NOTIFICATION_DISPLAY_TIME, NULL);
        return 0;

    case WM_TIMER:
        if (wParam == 1)
        {
            KillTimer(hwnd, 1);
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_LBUTTONDOWN:
        DestroyWindow(hwnd);
        return 0;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        RECT rect;
        GetClientRect(hwnd, &rect);

        HBRUSH bgBrush = CreateSolidBrush(RGB(45, 45, 48));
        FillRect(hdc, &rect, bgBrush);
        DeleteObject(bgBrush);

        return 1;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        NotificationWindow *notif = NULL;
        for (int i = 0; i < MAX_NOTIFICATIONS; i++)
        {
            if (g_notifications[i].hwnd == hwnd)
            {
                notif = &g_notifications[i];
                break;
            }
        }

        if (notif)
        {
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);

            SetBkMode(hdc, TRANSPARENT);

            // Border
            HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
            HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, 0, 0, clientRect.right, clientRect.bottom);
            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(borderPen);

            // Icon
            COLORREF iconColor;
            if (notif->type == NIIF_WARNING)
                iconColor = RGB(255, 165, 0);
            else if (notif->type == NIIF_ERROR)
                iconColor = RGB(220, 50, 50);
            else
                iconColor = RGB(80, 200, 120);

            HBRUSH iconBrush = CreateSolidBrush(iconColor);
            oldBrush = (HBRUSH)SelectObject(hdc, iconBrush);
            SelectObject(hdc, GetStockObject(NULL_PEN));
            Ellipse(hdc, 15, 15, 45, 45);
            SelectObject(hdc, oldBrush);
            DeleteObject(iconBrush);

            // Title
            HFONT oldFont = (HFONT)SelectObject(hdc, g_titleFont);
            RECT titleRect = {60, 12, clientRect.right - 10, 32};
            SetTextColor(hdc, RGB(255, 255, 255));
            DrawTextW(hdc, notif->title, -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            // Message
            SelectObject(hdc, g_messageFont);
            RECT messageRect = {60, 35, clientRect.right - 10, clientRect.bottom - 10};
            SetTextColor(hdc, RGB(200, 200, 200));
            DrawTextW(hdc, notif->message, -1, &messageRect, DT_LEFT | DT_WORDBREAK);

            SelectObject(hdc, oldFont);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
    {
        for (int i = 0; i < MAX_NOTIFICATIONS; i++)
        {
            if (g_notifications[i].hwnd == hwnd)
            {
                g_notifications[i].hwnd = NULL;
                break;
            }
        }
        RepositionNotifications();
        return 0;
    }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ProcessShowNotification(NotificationRequest *req)
{
    if (!req || !g_notificationSystemInitialized)
    {
        return;
    }

    int slot = FindFreeNotificationSlot();
    if (slot == -1)
    {
        if (g_notifications[0].hwnd && IsWindow(g_notifications[0].hwnd))
        {
            DestroyWindow(g_notifications[0].hwnd);
        }
        slot = 0;
    }

    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

    int x = workArea.right - NOTIFICATION_WIDTH - 10;
    int y = workArea.bottom - NOTIFICATION_HEIGHT - 10;

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        L"PingMonitorNotification",
        L"",
        WS_POPUP,
        x, y,
        NOTIFICATION_WIDTH, NOTIFICATION_HEIGHT,
        NULL, NULL,
        GetModuleHandle(NULL),
        NULL);

    if (hwnd)
    {
        SetLayeredWindowAttributes(hwnd, 0, 250, LWA_ALPHA);

        g_notifications[slot].hwnd = hwnd;
        wcsncpy(g_notifications[slot].title, req->title, 127);
        g_notifications[slot].title[127] = 0;
        wcsncpy(g_notifications[slot].message, req->message, 255);
        g_notifications[slot].message[255] = 0;
        g_notifications[slot].type = req->type;
        g_notifications[slot].startTime = GetTickCount();

        ShowWindow(hwnd, SW_SHOWNA);

        RepositionNotifications();
    }
}

void ShowCustomNotification(const wchar_t *title, const wchar_t *message, DWORD type)
{
    if (!g_mainHwnd)
    {
        return;
    }

    NotificationRequest *req = (NotificationRequest *)malloc(sizeof(NotificationRequest));
    if (!req)
    {
        return;
    }

    wcsncpy(req->title, title, 127);
    req->title[127] = 0;
    wcsncpy(req->message, message, 255);
    req->message[255] = 0;
    req->type = type;

    PostMessage(g_mainHwnd, WM_SHOW_NOTIFICATION, 0, (LPARAM)req);
}

void LoadNotificationSettings(void)
{
    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    wchar_t configPath[MAX_PATH];
    swprintf(configPath, MAX_PATH, L"%s\\config\\ping_config.ini", exeDir);

    g_notifSettings.enabled = GetPrivateProfileIntW(L"Settings", L"NotificationsEnabled",
                                                    DEFAULT_NOTIFICATION_ENABLED, configPath);
    g_notifSettings.cooldown = GetPrivateProfileIntW(L"Settings", L"NotificationCooldown",
                                                     DEFAULT_NOTIFICATION_COOLDOWN, configPath);
    g_notifSettings.notifyOnTimeout = GetPrivateProfileIntW(L"Settings", L"NotifyOnTimeout",
                                                            DEFAULT_NOTIFY_ON_TIMEOUT, configPath);
    g_notifSettings.notifyOnRecovery = GetPrivateProfileIntW(L"Settings", L"NotifyOnRecovery",
                                                             DEFAULT_NOTIFY_ON_RECOVERY, configPath);
    g_notifSettings.consecutiveFailuresThreshold = GetPrivateProfileIntW(L"Settings", L"ConsecutiveFailures",
                                                                         DEFAULT_CONSECUTIVE_FAILURES, configPath);
}

void ShowBalloonNotification(const wchar_t *title, const wchar_t *message, DWORD infoFlags)
{
    if (!g_notifSettings.enabled)
        return;

    ShowCustomNotification(title, message, infoFlags);
}

void CheckAndNotify(IPTarget *target)
{
    if (!g_notifSettings.enabled)
        return;

    time_t currentTime = time(NULL);
    time_t timeSinceLastNotification = currentTime - target->lastNotificationTime;

    struct tm *timeInfo = localtime(&currentTime);
    wchar_t timeStr[32];
    wcsftime(timeStr, 32, L"%H:%M:%S", timeInfo);

    wchar_t message[256];

    if (g_notifSettings.notifyOnTimeout &&
        target->consecutiveFailures == g_notifSettings.consecutiveFailuresThreshold)
    {
        if (timeSinceLastNotification < g_notifSettings.cooldown)
        {
            return;
        }

        swprintf(message, ARRAYSIZE(message),
                 L"[%s]\n%s (%s)\n%d회 연속 응답 없음",
                 timeStr, target->name, target->ip, target->consecutiveFailures);

        ShowBalloonNotification(L"⚠️ 네트워크 타임아웃", message, NIIF_WARNING);
        target->lastNotificationTime = currentTime;

        SaveNotificationLog(L"timeout", target->name, target->ip, timeStr);
    }
    else if (g_notifSettings.notifyOnRecovery &&
             target->previousOnline == 0 && target->online == 1 &&
             target->total > g_notifSettings.consecutiveFailuresThreshold)
    {
        swprintf(message, ARRAYSIZE(message),
                 L"[%s]\n%s (%s)\n연결 복구됨 (지연: %lu ms)",
                 timeStr, target->name, target->ip, target->latency);

        ShowBalloonNotification(L"✅ 네트워크 복구", message, NIIF_INFO);
        target->lastNotificationTime = currentTime;

        SaveNotificationLog(L"recovery", target->name, target->ip, timeStr);
    }

    target->previousOnline = target->online;
}

void SaveNotificationLog(const wchar_t *type, const wchar_t *name, const wchar_t *ip, const wchar_t *timeStr)
{
    EnterCriticalSection(&g_logLock);

    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    wchar_t logPath[MAX_PATH];
    swprintf(logPath, MAX_PATH, L"%s\\data\\notification_log.json", exeDir);

    wchar_t logData[50000] = L"[]";
    FILE *readFile = _wfopen(logPath, L"r, ccs=UTF-8");
    if (readFile)
    {
        wchar_t *ptr = logData;
        size_t remaining = 49999;
        while (remaining > 0 && fgetws(ptr, remaining, readFile))
        {
            size_t len = wcslen(ptr);
            ptr += len;
            remaining -= len;
        }
        *ptr = 0;
        fclose(readFile);
    }

    time_t now = time(NULL);
    struct tm *timeInfo = localtime(&now);
    wchar_t dateStr[32];
    wcsftime(dateStr, 32, L"%Y-%m-%d", timeInfo);

    wchar_t *insertPos = wcsstr(logData, L"]");
    if (insertPos)
    {
        BOOL isEmpty = (wcsstr(logData, L"[]") != NULL);

        wchar_t newEntry[512];
        swprintf(newEntry, 512,
                 L"%s\n  {\n"
                 L"    \"type\": \"%s\",\n"
                 L"    \"name\": \"%s\",\n"
                 L"    \"ip\": \"%s\",\n"
                 L"    \"time\": \"%s\",\n"
                 L"    \"date\": \"%s\"\n"
                 L"  }\n]",
                 isEmpty ? L"" : L",",
                 type, name, ip, timeStr, dateStr);

        size_t beforeLen = insertPos - logData;
        wchar_t result[51000];
        wcsncpy(result, logData, beforeLen);
        result[beforeLen] = 0;
        wcscat(result, newEntry);

        wchar_t tmpPath[MAX_PATH];
        swprintf(tmpPath, MAX_PATH, L"%s\\data\\notification_log.json.tmp", exeDir);

        FILE *writeFile = _wfopen(tmpPath, L"w, ccs=UTF-8");
        if (writeFile)
        {
            fwprintf(writeFile, L"%s", result);
            fclose(writeFile);

            DeleteFileW(logPath);
            MoveFileW(tmpPath, logPath);
        }
    }

    LeaveCriticalSection(&g_logLock);
}