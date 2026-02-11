// ping_monitor_webview.c - Main Entry Point (v2.7 - 365일 무중단 운영)

#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdi32.lib")

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
// v2.7: 자동 재시작 설정
// ============================================================================

#define RESTART_FLAG_FILE L"ping_monitor_restart.flag"
#define MAX_RESTART_COUNT 5
#define RESTART_COOLDOWN_SEC 60

static int g_restartCount = 0;
static time_t g_lastRestartTime = 0;

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

// v2.7: 자동 재시작 함수
void CreateRestartFlag(void);
BOOL CheckAndClearRestartFlag(void);
void RequestRestart(void);
LONG WINAPI CrashHandler(EXCEPTION_POINTERS *exInfo);

// ============================================================================
// v2.7: SEH 예외 처리 (크래시 핸들러)
// ============================================================================

LONG WINAPI CrashHandler(EXCEPTION_POINTERS *exInfo)
{
    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    // 크래시 로그 기록
    wchar_t logPath[MAX_PATH];
    swprintf(logPath, MAX_PATH, L"%s\\data\\crash_log.txt", exeDir);

    FILE *fp = _wfopen(logPath, L"a, ccs=UTF-8");
    if (fp)
    {
        time_t now = time(NULL);
        wchar_t timeStr[64];
        wcsftime(timeStr, 64, L"%Y-%m-%d %H:%M:%S", localtime(&now));

        fwprintf(fp, L"[%s] 크래시 발생 - 예외 코드: 0x%08X, 주소: 0x%p\n",
                 timeStr,
                 exInfo->ExceptionRecord->ExceptionCode,
                 exInfo->ExceptionRecord->ExceptionAddress);
        fclose(fp);
    }

    wprintf(L"[크래시 핸들러] 예외 발생 - 자동 재시작 시도\n");

    // 재시작 플래그 생성
    CreateRestartFlag();

    // 정리 작업
    g_isRunning = FALSE;
    StopHttpServer();
    CleanupNotificationSystem();
    CleanupNetworkModule();
    CleanupOutageSystem();

    // 자기 자신 재시작
    RequestRestart();

    return EXCEPTION_EXECUTE_HANDLER;
}

/**
 * 재시작 플래그 파일 생성
 */
void CreateRestartFlag(void)
{
    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    wchar_t flagPath[MAX_PATH];
    swprintf(flagPath, MAX_PATH, L"%s\\%s", exeDir, RESTART_FLAG_FILE);

    FILE *fp = _wfopen(flagPath, L"w");
    if (fp)
    {
        fwprintf(fp, L"%d", g_restartCount + 1);
        fclose(fp);
    }
}

/**
 * 재시작 플래그 확인 및 삭제
 */
BOOL CheckAndClearRestartFlag(void)
{
    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    wchar_t flagPath[MAX_PATH];
    swprintf(flagPath, MAX_PATH, L"%s\\%s", exeDir, RESTART_FLAG_FILE);

    if (GetFileAttributesW(flagPath) != INVALID_FILE_ATTRIBUTES)
    {
        // 재시작 횟수 읽기
        FILE *fp = _wfopen(flagPath, L"r");
        if (fp)
        {
            fwscanf(fp, L"%d", &g_restartCount);
            fclose(fp);
        }

        // 플래그 파일 삭제
        DeleteFileW(flagPath);

        wprintf(L"[자동 재시작] 이전 크래시로 인한 재시작 #%d\n", g_restartCount);
        return TRUE;
    }

    return FALSE;
}

/**
 * 프로세스 재시작 요청
 */
void RequestRestart(void)
{
    time_t now = time(NULL);

    // 쿨다운 체크 (너무 빠른 재시작 방지)
    if (g_lastRestartTime > 0 && difftime(now, g_lastRestartTime) < RESTART_COOLDOWN_SEC)
    {
        wprintf(L"[자동 재시작] 쿨다운 중 - 재시작 건너뜀\n");
        return;
    }

    // 최대 재시작 횟수 체크
    if (g_restartCount >= MAX_RESTART_COUNT)
    {
        wprintf(L"[자동 재시작] 최대 재시작 횟수 초과 (%d회) - 중단\n", MAX_RESTART_COUNT);
        MessageBoxW(NULL,
                    L"프로그램이 반복적으로 크래시되고 있습니다.\n"
                    L"문제 해결 후 수동으로 다시 시작해 주세요.",
                    L"자동 재시작 중단", MB_OK | MB_ICONERROR);
        return;
    }

    g_lastRestartTime = now;

    // 자기 자신 재실행
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    STARTUPINFOW si = {sizeof(si)};
    PROCESS_INFORMATION pi;

    if (CreateProcessW(exePath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        wprintf(L"[자동 재시작] 새 프로세스 시작됨 (PID: %lu)\n", pi.dwProcessId);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else
    {
        wprintf(L"[자동 재시작] 프로세스 재시작 실패 (에러: %lu)\n", GetLastError());
    }
}

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
        SendMessageW(hwnd, WM_CLOSE, 0, 0);
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
            if (!g_notifSettings.enabled)
            {
                CleanupNotificationSystem();
            }
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

    case WM_SHOW_NOTIFICATION:
    {
        NotificationRequest *req = (NotificationRequest *)lParam;
        if (req)
        {
            ProcessShowNotification(req);
            free(req);
        }
        break;
    }

    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER_BROWSER_CHECK);
        CleanupBrowserMonitor();
        CleanupNotificationSystem();
        RemoveTrayIcon();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ============================================================================
// Main Entry Point (v2.7 - SEH 예외 처리 적용)
// ============================================================================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // v2.7: SEH 크래시 핸들러 등록
    SetUnhandledExceptionFilter(CrashHandler);

    // v2.7: 재시작 플래그 확인
    CheckAndClearRestartFlag();

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        MessageBoxW(NULL, L"Winsock 초기화 실패", L"오류", MB_OK | MB_ICONERROR);
        return 1;
    }

    InitializeCriticalSection(&g_logLock);
    InitOutageSystem();
    InitNotificationSystem();

    // v2.7: 네트워크 모듈 초기화 (정적 리소스)
    if (!InitNetworkModule())
    {
        wprintf(L"[경고] 네트워크 모듈 최적화 초기화 실패 - 기본 모드로 실행\n");
    }

    if (!CheckRequiredFiles())
    {
        CleanupNetworkModule();
        CleanupOutageSystem();
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
        CleanupNetworkModule();
        CleanupOutageSystem();
        DeleteCriticalSection(&g_logLock);
        WSACleanup();
        return 1;
    }

    wprintf(L"===========================================\n");
    wprintf(L"Ping Monitor v2.7 - 365일 무중단 운영\n");
    wprintf(L"===========================================\n");
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
                CleanupNetworkModule();
                CleanupOutageSystem();
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
            CleanupNetworkModule();
            CleanupOutageSystem();
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
        CleanupNetworkModule();
        CleanupOutageSystem();
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
        CleanupNetworkModule();
        CleanupOutageSystem();
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

    wprintf(L"Browser launched successfully\n");
    wprintf(L"[365일 운영] 크래시 핸들러 활성화됨\n");

    // v2.7: 재시작 카운터 리셋 타이머 (정상 운영 1시간 후)
    if (g_restartCount > 0)
    {
        wprintf(L"[자동 재시작] 정상 운영 확인 시 재시작 카운터 초기화\n");
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 정상 종료 시 정리
    StopMonitoring();
    StopHttpServer();
    CleanupNotificationSystem();
    CleanupNetworkModule();
    CleanupOutageSystem();
    DeleteCriticalSection(&g_logLock);
    WSACleanup();

    return (int)msg.wParam;
}