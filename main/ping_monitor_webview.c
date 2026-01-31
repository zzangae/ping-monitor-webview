// ping_monitor_webview.c - Main Entry Point (Modularized v2.6)

#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")

// Module headers
#include "module/types.h"
#include "module/tray.h"
#include "module/network.h"
#include "module/notification.h"
#include "module/config.h"
#include "module/port.h"

// External modules
#include "http_server.h"
#include "outage.h"
#include "browser_monitor.h"

// ============================================================================
// Global Variables (Definitions)
// ============================================================================

IPTarget g_targets[MAX_IP_COUNT];
int g_targetCount = 0;
TimeSettings g_timeSettings = {1000, 0, TRUE};
BOOL g_isRunning = FALSE;
HWND g_hwnd = NULL;
HWND g_mainHwnd = NULL;
NOTIFYICONDATAW g_nid = {0};
wchar_t g_exePath[MAX_PATH] = {0};
int g_currentPort = HTTP_PORT;
DWORD g_browserStartTime = 0;

NotificationSettings g_notifSettings = {
    DEFAULT_NOTIFICATION_ENABLED,
    DEFAULT_NOTIFICATION_COOLDOWN,
    DEFAULT_NOTIFY_ON_TIMEOUT,
    DEFAULT_NOTIFY_ON_RECOVERY,
    DEFAULT_CONSECUTIVE_FAILURES};

CRITICAL_SECTION g_logLock;

// ============================================================================
// Function Declarations
// ============================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CheckRequiredFiles(void);
void KillPreviousInstance(void);
void ReloadConfigAndRestart(HWND hwnd);

// ============================================================================
// Required Files Check
// ============================================================================

BOOL CheckRequiredFiles(void)
{
    wchar_t exePath[MAX_PATH];
    wchar_t exeDir[MAX_PATH];

    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wcscpy(exeDir, exePath);
    PathRemoveFileSpecW(exeDir);

    const wchar_t *requiredFiles[] = {
        L"web\\graph.html",
        L"web\\css\\variables.css",
        L"web\\css\\base.css",
        L"web\\css\\components.css",
        L"web\\css\\dashboard.css",
        L"web\\css\\notifications.css",
        L"web\\css\\responsive.css"};

    wchar_t missingFiles[2048] = L"다음 필수 파일이 없습니다:\n\n";
    BOOL allFilesExist = TRUE;

    for (int i = 0; i < sizeof(requiredFiles) / sizeof(requiredFiles[0]); i++)
    {
        wchar_t filePath[MAX_PATH];
        swprintf(filePath, MAX_PATH, L"%s\\%s", exeDir, requiredFiles[i]);

        if (GetFileAttributesW(filePath) == INVALID_FILE_ATTRIBUTES)
        {
            wcscat(missingFiles, L"  - ");
            wcscat(missingFiles, filePath);
            wcscat(missingFiles, L"\n");
            allFilesExist = FALSE;
        }
    }

    if (!allFilesExist)
    {
        wcscat(missingFiles, L"\n\n현재 확인된 경로:\n");
        wcscat(missingFiles, exeDir);

        MessageBoxW(NULL, missingFiles, L"필수 파일 누락", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    return TRUE;
}

// ============================================================================
// Kill Previous Instance
// ============================================================================

void KillPreviousInstance(void)
{
    HWND hwnd = FindWindowW(L"PingMonitorClass", NULL);
    if (hwnd)
    {
        wprintf(L"이전 인스턴스 종료 중...\n");
        SendMessage(hwnd, WM_CLOSE, 0, 0);
        Sleep(500);
    }
}

// ============================================================================
// Reload Config and Restart
// ============================================================================

void ReloadConfigAndRestart(HWND hwnd)
{
    BOOL wasRunning = g_isRunning;
    g_isRunning = FALSE;
    Sleep(500);

    int oldCount = g_targetCount;

    g_targetCount = 0;
    LoadConfig();

    wchar_t exePath[MAX_PATH];
    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wcscpy(exeDir, exePath);
    PathRemoveFileSpecW(exeDir);

    wchar_t configPath[MAX_PATH];
    swprintf(configPath, MAX_PATH, L"%s\\config\\ping_config.ini", exeDir);
    LoadOutageConfig(configPath);

    wchar_t msg[512];
    if (g_targetCount > 0)
    {
        swprintf(msg, 512,
                 L"설정을 다시 불러왔습니다.\n\n"
                 L"이전 IP 개수: %d\n"
                 L"현재 IP 개수: %d\n\n"
                 L"모니터링이 자동으로 재시작됩니다.",
                 oldCount, g_targetCount);

        MessageBoxW(hwnd, msg, L"설정 불러오기 완료", MB_OK | MB_ICONINFORMATION);

        if (wasRunning)
        {
            g_isRunning = TRUE;
        }
    }
    else
    {
        MessageBoxW(hwnd,
                    L"설정 파일에서 IP를 찾을 수 없습니다.\n"
                    L"config\\ping_config.ini 또는 config\\int_config.ini를 확인하세요.",
                    L"설정 불러오기 실패",
                    MB_OK | MB_ICONWARNING);
    }
}

// ============================================================================
// Window Procedure
// ============================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        InitTrayIcon(hwnd);
        break;

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP)
        {
            ShowTrayMenu(hwnd);
        }
        else if (lParam == WM_LBUTTONDBLCLK)
        {
            wchar_t url[256];
            swprintf(url, 256, L"http://localhost:%d/web/graph.html", g_currentPort);
            OpenBrowser(url);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_TRAY_START:
            if (g_isRunning)
            {
                StopMonitoring();
            }
            else
            {
                StartMonitoring();
            }
            break;

        case ID_TRAY_BROWSER:
        {
            wchar_t url[256];
            swprintf(url, 256, L"http://localhost:%d/web/graph.html", g_currentPort);
            g_browserStartTime = GetTickCount();
            OpenBrowser(url);
            break;
        }

        case ID_TRAY_NOTIFICATIONS:
            g_notifSettings.enabled = !g_notifSettings.enabled;
            wprintf(L"알림: %s\n", g_notifSettings.enabled ? L"활성화" : L"비활성화");
            break;

        case ID_TRAY_RELOAD_CONFIG:
            ReloadConfigAndRestart(hwnd);
            break;

        case ID_TRAY_CHANGE_PORT:
            ChangeServerPort(hwnd);
            break;

        case ID_TRAY_EXIT:
            PostQuitMessage(0);
            break;
        }
        break;

    case WM_TIMER:
        if (wParam == ID_TIMER_BROWSER_CHECK)
        {
            // Browser monitoring temporarily disabled
        }
        break;

    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER_BROWSER_CHECK);
        CleanupBrowserMonitor();
        RemoveTrayIcon();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ============================================================================
// Main Entry Point
// ============================================================================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        MessageBoxW(NULL, L"Winsock 초기화 실패", L"오류", MB_OK | MB_ICONERROR);
        return 1;
    }

    InitializeCriticalSection(&g_logLock);
    InitOutageSystem();

    if (!CheckRequiredFiles())
    {
        DeleteCriticalSection(&g_logLock);
        WSACleanup();
        return 1;
    }

    KillPreviousInstance();

    wchar_t exePath[MAX_PATH];
    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wcscpy(exeDir, exePath);
    PathRemoveFileSpecW(exeDir);

    LoadConfig();

    wchar_t configPath[MAX_PATH];
    swprintf(configPath, MAX_PATH, L"%s\\config\\ping_config.ini", exeDir);
    LoadOutageConfig(configPath);

    if (g_targetCount == 0)
    {
        MessageBoxW(NULL, L"설정 파일에 IP가 없습니다.", L"오류", MB_OK | MB_ICONERROR);
        DeleteCriticalSection(&g_logLock);
        WSACleanup();
        return 1;
    }

    wprintf(L"실행 경로: %s\n", exeDir);
    wprintf(L"HTTP 서버 시작 시도 (포트: %d)...\n", HTTP_PORT);

    if (!StartHttpServer(HTTP_PORT, exeDir))
    {
        wprintf(L"기본 포트 %d 사용 불가. 대체 포트 검색 중...\n", HTTP_PORT);

        int availablePort = FindAvailablePort(HTTP_PORT_MIN, HTTP_PORT_MAX);
        if (availablePort != -1)
        {
            wprintf(L"사용 가능한 포트 발견: %d\n", availablePort);
            if (StartHttpServer(availablePort, exeDir))
            {
                g_currentPort = availablePort;
                wprintf(L"HTTP 서버 시작 성공: http://localhost:%d\n", availablePort);
            }
            else
            {
                MessageBoxW(NULL, L"HTTP 서버 시작 실패", L"오류", MB_OK | MB_ICONERROR);
                DeleteCriticalSection(&g_logLock);
                WSACleanup();
                return 1;
            }
        }
        else
        {
            wchar_t errorMsg[512];
            swprintf(errorMsg, 512,
                     L"HTTP 서버 시작 실패\n\n"
                     L"포트 %d-%d 범위에서 사용 가능한 포트를 찾을 수 없습니다.",
                     HTTP_PORT_MIN, HTTP_PORT_MAX);
            MessageBoxW(NULL, errorMsg, L"오류", MB_OK | MB_ICONERROR);
            DeleteCriticalSection(&g_logLock);
            WSACleanup();
            return 1;
        }
    }
    else
    {
        g_currentPort = HTTP_PORT;
        wprintf(L"HTTP 서버 시작 성공: http://localhost:%d\n", HTTP_PORT);
    }

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"PingMonitorClass";

    if (!RegisterClassW(&wc))
    {
        MessageBoxW(NULL, L"윈도우 클래스 등록 실패", L"오류", MB_OK | MB_ICONERROR);
        StopHttpServer();
        DeleteCriticalSection(&g_logLock);
        WSACleanup();
        return 1;
    }

    g_hwnd = CreateWindowW(
        L"PingMonitorClass",
        L"Ping Monitor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 300,
        NULL, NULL, hInstance, NULL);

    if (!g_hwnd)
    {
        MessageBoxW(NULL, L"윈도우 생성 실패", L"오류", MB_OK | MB_ICONERROR);
        StopHttpServer();
        DeleteCriticalSection(&g_logLock);
        WSACleanup();
        return 1;
    }

    g_mainHwnd = g_hwnd;

    ShowWindow(g_hwnd, SW_HIDE);

    StartMonitoring();

    wchar_t url[256];
    swprintf(url, 256, L"http://localhost:%d/web/graph.html", g_currentPort);
    g_browserStartTime = GetTickCount();
    OpenBrowser(url);

    wprintf(L"Browser launched successfully (auto-close disabled)\n");

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    StopMonitoring();
    StopHttpServer();
    DeleteCriticalSection(&g_logLock);
    WSACleanup();

    return (int)msg.wParam;
}