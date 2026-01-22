/**
 * PING MONITOR with WebView2 Graph
 *
 * WebView2를 사용한 실시간 네트워크 모니터링 + 그래프 시각화
 *
 * 컴파일 방법 (MinGW-w64):
 *   gcc -o ping_monitor.exe ping_monitor_webview.c -lole32 -loleaut32 -luuid -lws2_32 -liphlpapi -mwindows -municode
 *
 * 필수 요구사항:
 *   - Windows 10 버전 1803 이상 또는 Windows 11
 *   - Microsoft Edge WebView2 Runtime (대부분 PC에 기본 설치됨)
 *   - graph.html 파일 (같은 폴더에 위치)
 *
 * 아키텍처:
 *   ┌─────────────────────────────────────────────┐
 *   │  ping_monitor.exe                           │
 *   │  ┌─────────────┐    ┌───────────────────┐  │
 *   │  │  C 핑 엔진  │───▶│  WebView2 (HTML)  │  │
 *   │  │  - ICMP     │    │  - Chart.js       │  │
 *   │  │  - 타이머   │◀───│  - 실시간 그래프  │  │
 *   │  └─────────────┘    └───────────────────┘  │
 *   └─────────────────────────────────────────────┘
 */

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

// Windows 버전 타겟 (Windows 10 이상)
#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tchar.h>

#include "http_server.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "shell32.lib")

// ============================================================================
// WebView2 헤더 (간소화된 인터페이스)
// 실제 프로젝트에서는 WebView2.h 사용
// ============================================================================

// WebView2 로더 DLL 사용을 위한 선언
typedef HRESULT(STDAPICALLTYPE *CreateCoreWebView2EnvironmentWithOptionsFunc)(
    PCWSTR browserExecutableFolder,
    PCWSTR userDataFolder,
    void *environmentOptions,
    void *environmentCreatedHandler);

// ============================================================================
// 상수 정의
// ============================================================================
#define MAX_IP_COUNT 50
#define MAX_IP_LEN 64
#define MAX_NAME_LEN 64
#define MAX_PATH_LEN 512
#define PING_TIMEOUT 1000
#define PING_INTERVAL 1000 // 1초

#define IDT_PING_TIMER 1001
#define IDT_UPDATE_TIMER 1002

// 트레이 아이콘
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 3001
#define ID_TRAY_OPEN_BROWSER 3002
#define ID_TRAY_TOGGLE 3003

// HTTP 서버
#define HTTP_PORT 8080
#define HTTP_BUFFER_SIZE 8192

// ============================================================================
// 데이터 구조체
// ============================================================================
typedef struct
{
    WCHAR ip[MAX_IP_LEN];
    WCHAR name[MAX_NAME_LEN];
    int latency;  // -1 = timeout
    int isOnline; // 1 = online, 0 = timeout, -1 = ready

    // 히스토리 (최근 60개)
    int history[60];
    int historyIndex;
    int historyCount;

    // 통계
    int totalPings;
    int successPings;
    int minLatency;
    int maxLatency;
    double avgLatency;
} IPTarget;

typedef struct
{
    int hours;
    int minutes;
    int seconds;
    BOOL loopMode;
} TimeSettings;

// ============================================================================
// 전역 변수
// ============================================================================
HWND g_hMainWnd = NULL;
HWND g_hWebView = NULL; // WebView2가 생성될 자식 윈도우
HINSTANCE g_hInstance = NULL;

IPTarget g_targets[MAX_IP_COUNT];
int g_targetCount = 0;

BOOL g_isRunning = FALSE;
int g_elapsedSeconds = 0;
int g_totalSeconds = 0;
TimeSettings g_timeSettings = {0, 0, 0, TRUE}; // 무한 루프

HANDLE g_hIcmp = NULL;
WCHAR g_exePath[MAX_PATH_LEN] = {0};

// WebView2 관련
BOOL g_webViewReady = FALSE;

// 트레이 아이콘
NOTIFYICONDATAW g_nid = {0};

// ============================================================================
// 함수 선언
// ============================================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void LoadConfig(void);
void SaveConfig(void);
BOOL DoPing(const WCHAR *ipAddress, int *latency);
void UpdateTarget(int index);
void SendDataToWebView(void);
void InitializeWebView2(HWND hWnd);
WCHAR *BuildJsonData(void);
void StartMonitoring(void);
void StopMonitoring(void);
void InitTrayIcon(HWND hWnd);
void RemoveTrayIcon(void);
void ShowTrayMenu(HWND hWnd);
void OpenBrowser(void);
void KillPreviousInstance(void);

// ============================================================================
// 이전 인스턴스 종료
// ============================================================================
void KillPreviousInstance(void)
{
    DWORD currentPid = GetCurrentProcessId();

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe))
    {
        do
        {
            // ping_monitor.exe 찾기 (자신 제외)
            if (_wcsicmp(pe.szExeFile, L"ping_monitor.exe") == 0 &&
                pe.th32ProcessID != currentPid)
            {

                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProcess)
                {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                    // 프로세스 종료 대기
                    Sleep(500);
                }
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
}

// ============================================================================
// 설정 파일 로드
// ============================================================================
void LoadConfig(void)
{
    WCHAR configPath[MAX_PATH_LEN];
    swprintf(configPath, MAX_PATH_LEN, L"%s\\ping_config.ini", g_exePath);

    FILE *fp = _wfopen(configPath, L"r, ccs=UTF-8");
    if (!fp)
    {
        // 기본 설정
        wcscpy(g_targets[0].ip, L"8.8.8.8");
        wcscpy(g_targets[0].name, L"Google DNS");
        g_targets[0].minLatency = 9999;

        wcscpy(g_targets[1].ip, L"1.1.1.1");
        wcscpy(g_targets[1].name, L"Cloudflare");
        g_targets[1].minLatency = 9999;

        wcscpy(g_targets[2].ip, L"168.126.63.1");
        wcscpy(g_targets[2].name, L"KT DNS");
        g_targets[2].minLatency = 9999;

        wcscpy(g_targets[3].ip, L"192.168.1.1");
        wcscpy(g_targets[3].name, L"Gateway");
        g_targets[3].minLatency = 9999;

        g_targetCount = 4;
        return;
    }

    WCHAR line[256];
    g_targetCount = 0;

    while (fgetws(line, 256, fp) && g_targetCount < MAX_IP_COUNT)
    {
        // 줄바꿈 제거
        WCHAR *newline = wcschr(line, L'\n');
        if (newline)
            *newline = L'\0';
        newline = wcschr(line, L'\r');
        if (newline)
            *newline = L'\0';

        // 빈 줄 또는 주석 무시
        if (line[0] == L'\0' || line[0] == L'#' || line[0] == L';')
            continue;

        // IP,이름 파싱
        WCHAR *comma = wcschr(line, L',');
        if (comma)
        {
            *comma = L'\0';
            wcsncpy(g_targets[g_targetCount].ip, line, MAX_IP_LEN - 1);
            wcsncpy(g_targets[g_targetCount].name, comma + 1, MAX_NAME_LEN - 1);
        }
        else
        {
            wcsncpy(g_targets[g_targetCount].ip, line, MAX_IP_LEN - 1);
            wcscpy(g_targets[g_targetCount].name, L"Unknown");
        }

        g_targets[g_targetCount].minLatency = 9999;
        g_targets[g_targetCount].isOnline = -1; // Ready 상태
        g_targetCount++;
    }

    fclose(fp);

    if (g_targetCount == 0)
    {
        // 설정 파일이 비어있으면 기본값
        wcscpy(g_targets[0].ip, L"8.8.8.8");
        wcscpy(g_targets[0].name, L"Google DNS");
        g_targets[0].minLatency = 9999;
        g_targetCount = 1;
    }
}

// ============================================================================
// 핑 실행
// ============================================================================
BOOL DoPing(const WCHAR *ipAddress, int *latency)
{
    *latency = -1;

    if (!g_hIcmp)
    {
        g_hIcmp = IcmpCreateFile();
        if (g_hIcmp == INVALID_HANDLE_VALUE)
        {
            g_hIcmp = NULL;
            return FALSE;
        }
    }

    // IP 주소 변환
    char ipAnsi[MAX_IP_LEN];
    WideCharToMultiByte(CP_ACP, 0, ipAddress, -1, ipAnsi, MAX_IP_LEN, NULL, NULL);

    ULONG ipAddr = inet_addr(ipAnsi);
    if (ipAddr == INADDR_NONE)
    {
        // DNS 해석 시도
        struct addrinfo hints = {0};
        struct addrinfo *result = NULL;
        hints.ai_family = AF_INET;

        if (getaddrinfo(ipAnsi, NULL, &hints, &result) == 0 && result)
        {
            ipAddr = ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
            freeaddrinfo(result);
        }
        else
        {
            return FALSE;
        }
    }

    // ICMP 요청
    char sendData[32] = "PingMonitor";
    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData) + 8;
    BYTE *replyBuffer = (BYTE *)malloc(replySize);

    if (!replyBuffer)
        return FALSE;

    DWORD ret = IcmpSendEcho(g_hIcmp, ipAddr, sendData, sizeof(sendData),
                             NULL, replyBuffer, replySize, PING_TIMEOUT);

    if (ret > 0)
    {
        PICMP_ECHO_REPLY reply = (PICMP_ECHO_REPLY)replyBuffer;
        if (reply->Status == IP_SUCCESS)
        {
            *latency = (int)reply->RoundTripTime;
            free(replyBuffer);
            return TRUE;
        }
    }

    free(replyBuffer);
    return FALSE;
}

// ============================================================================
// 타겟 업데이트
// ============================================================================
void UpdateTarget(int index)
{
    if (index < 0 || index >= g_targetCount)
        return;

    IPTarget *target = &g_targets[index];
    int latency;

    BOOL success = DoPing(target->ip, &latency);

    target->totalPings++;

    if (success)
    {
        target->latency = latency;
        target->isOnline = 1;
        target->successPings++;

        // 통계 업데이트
        if (latency < target->minLatency)
            target->minLatency = latency;
        if (latency > target->maxLatency)
            target->maxLatency = latency;
        target->avgLatency = ((target->avgLatency * (target->successPings - 1)) + latency) / target->successPings;
    }
    else
    {
        target->latency = -1;
        target->isOnline = 0;
    }

    // 히스토리 추가
    target->history[target->historyIndex] = target->latency;
    target->historyIndex = (target->historyIndex + 1) % 60;
    if (target->historyCount < 60)
        target->historyCount++;
}

// ============================================================================
// JSON 데이터 생성
// ============================================================================
WCHAR *BuildJsonData(void)
{
    static WCHAR json[32768]; // 충분히 큰 버퍼

    // JSON 시작
    int pos = swprintf(json, 32768, L"{\"running\":%s,\"elapsed\":%d,\"total\":%d,\"loop\":%s,\"targets\":[",
                       g_isRunning ? L"true" : L"false",
                       g_elapsedSeconds,
                       g_totalSeconds,
                       g_timeSettings.loopMode ? L"true" : L"false");

    for (int i = 0; i < g_targetCount; i++)
    {
        IPTarget *t = &g_targets[i];

        if (i > 0)
        {
            pos += swprintf(json + pos, 32768 - pos, L",");
        }

        // 히스토리 배열 생성
        WCHAR historyStr[512] = L"[";
        int histLen = 1;
        for (int j = 0; j < t->historyCount; j++)
        {
            int idx = (t->historyIndex - t->historyCount + j + 60) % 60;
            if (j > 0)
                histLen += swprintf(historyStr + histLen, 512 - histLen, L",");
            histLen += swprintf(historyStr + histLen, 512 - histLen, L"%d", t->history[idx]);
        }
        histLen += swprintf(historyStr + histLen, 512 - histLen, L"]");

        // 타겟 JSON
        pos += swprintf(json + pos, 32768 - pos,
                        L"{\"ip\":\"%s\",\"name\":\"%s\",\"latency\":%d,\"online\":%d,"
                        L"\"total\":%d,\"success\":%d,\"min\":%d,\"max\":%d,\"avg\":%.1f,\"history\":%s}",
                        t->ip, t->name, t->latency, t->isOnline,
                        t->totalPings, t->successPings,
                        t->minLatency == 9999 ? 0 : t->minLatency,
                        t->maxLatency, t->avgLatency, historyStr);
    }

    pos += swprintf(json + pos, 32768 - pos, L"]}");

    return json;
}

// ============================================================================
// WebView2로 데이터 전송 (나중에 구현)
// ============================================================================
void SendDataToWebView(void)
{
    if (!g_webViewReady)
        return;

    WCHAR *json = BuildJsonData();

    // 원자적 쓰기: 임시 파일에 쓰고 이름 변경
    WCHAR tempPath[MAX_PATH_LEN];
    WCHAR dataPath[MAX_PATH_LEN];
    swprintf(tempPath, MAX_PATH_LEN, L"%s\\ping_data.tmp", g_exePath);
    swprintf(dataPath, MAX_PATH_LEN, L"%s\\ping_data.json", g_exePath);

    FILE *fp = _wfopen(tempPath, L"w, ccs=UTF-8");
    if (fp)
    {
        fwprintf(fp, L"%s", json);
        fclose(fp);

        // 기존 파일 삭제 후 이름 변경
        _wremove(dataPath);
        _wrename(tempPath, dataPath);
    }
}

// ============================================================================
// 모니터링 시작
// ============================================================================
void StartMonitoring(void)
{
    if (g_isRunning)
        return;

    // 통계 초기화
    for (int i = 0; i < g_targetCount; i++)
    {
        g_targets[i].totalPings = 0;
        g_targets[i].successPings = 0;
        g_targets[i].minLatency = 9999;
        g_targets[i].maxLatency = 0;
        g_targets[i].avgLatency = 0;
        g_targets[i].historyIndex = 0;
        g_targets[i].historyCount = 0;
        g_targets[i].isOnline = -1;
        memset(g_targets[i].history, -1, sizeof(g_targets[i].history));
    }

    g_totalSeconds = g_timeSettings.hours * 3600 +
                     g_timeSettings.minutes * 60 +
                     g_timeSettings.seconds;
    g_elapsedSeconds = 0;
    g_isRunning = TRUE;

    SetTimer(g_hMainWnd, IDT_PING_TIMER, PING_INTERVAL, NULL);
}

// ============================================================================
// 모니터링 중지
// ============================================================================
void StopMonitoring(void)
{
    if (!g_isRunning)
        return;

    g_isRunning = FALSE;
    KillTimer(g_hMainWnd, IDT_PING_TIMER);

    SendDataToWebView(); // 마지막 상태 전송
}

// ============================================================================
// 트레이 아이콘 초기화
// ============================================================================
void InitTrayIcon(HWND hWnd)
{
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = hWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy(g_nid.szTip, L"Ping Monitor - 실행 중");

    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

// ============================================================================
// 트레이 아이콘 제거
// ============================================================================
void RemoveTrayIcon(void)
{
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

// ============================================================================
// 트레이 메뉴 표시
// ============================================================================
void ShowTrayMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();

    if (g_isRunning)
    {
        AppendMenuW(hMenu, MF_STRING, ID_TRAY_TOGGLE, L"⏸️ 모니터링 일시정지");
    }
    else
    {
        AppendMenuW(hMenu, MF_STRING, ID_TRAY_TOGGLE, L"▶️ 모니터링 시작");
    }
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_OPEN_BROWSER, L"🌐 브라우저에서 열기");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"❌ 종료");

    // 메뉴를 제대로 표시하기 위해 필요
    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON,
                   pt.x, pt.y, 0, hWnd, NULL);
    PostMessage(hWnd, WM_NULL, 0, 0);

    DestroyMenu(hMenu);
}

// ============================================================================
// 브라우저에서 열기
// ============================================================================
void OpenBrowser(void)
{
    WCHAR url[256];
    swprintf(url, 256, L"http://localhost:%d/graph.html", GetHttpServerPort());
    ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);
}

// ============================================================================
// 윈도우 프로시저
// ============================================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        // WebView2 초기화는 별도 처리 필요
        // 여기서는 간소화를 위해 데이터 파일 방식 사용
        g_webViewReady = TRUE;
        return 0;
    }

    case WM_SIZE:
    {
        // 윈도우 크기 변경 시 WebView도 조정
        if (g_hWebView)
        {
            RECT rc;
            GetClientRect(hWnd, &rc);
            SetWindowPos(g_hWebView, NULL, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
        }
        return 0;
    }

    case WM_TIMER:
    {
        if (wParam == IDT_PING_TIMER && g_isRunning)
        {
            // 모든 타겟에 핑
            for (int i = 0; i < g_targetCount; i++)
            {
                UpdateTarget(i);
            }

            g_elapsedSeconds++;

            // 시간 체크
            if (!g_timeSettings.loopMode && g_elapsedSeconds >= g_totalSeconds)
            {
                StopMonitoring();
            }

            SendDataToWebView();
        }
        return 0;
    }

    case WM_KEYDOWN:
    {
        if (wParam == VK_F5)
        {
            // F5: 시작/중지 토글
            if (g_isRunning)
            {
                StopMonitoring();
            }
            else
            {
                StartMonitoring();
            }
        }
        else if (wParam == VK_ESCAPE)
        {
            // ESC: 중지
            StopMonitoring();
        }
        return 0;
    }

    // 트레이 아이콘 메시지
    case WM_TRAYICON:
    {
        if (lParam == WM_RBUTTONUP)
        {
            ShowTrayMenu(hWnd);
        }
        else if (lParam == WM_LBUTTONDBLCLK)
        {
            OpenBrowser();
        }
        return 0;
    }

    // 메뉴 명령 처리
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case ID_TRAY_EXIT:
            DestroyWindow(hWnd);
            break;
        case ID_TRAY_OPEN_BROWSER:
            OpenBrowser();
            break;
        case ID_TRAY_TOGGLE:
            if (g_isRunning)
            {
                StopMonitoring();
                // 트레이 아이콘 툴팁 업데이트
                wcscpy(g_nid.szTip, L"Ping Monitor - 일시정지");
                Shell_NotifyIconW(NIM_MODIFY, &g_nid);
            }
            else
            {
                StartMonitoring();
                wcscpy(g_nid.szTip, L"Ping Monitor - 실행 중");
                Shell_NotifyIconW(NIM_MODIFY, &g_nid);
            }
            break;
        }
        return 0;
    }

    case WM_DESTROY:
    {
        StopMonitoring();
        StopHttpServer();
        RemoveTrayIcon();
        if (g_hIcmp)
        {
            IcmpCloseHandle(g_hIcmp);
            g_hIcmp = NULL;
        }
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ============================================================================
// 메인 함수
// ============================================================================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nCmdShow)
{
    g_hInstance = hInstance;

    // 이전 인스턴스 종료
    KillPreviousInstance();

    // 실행 파일 경로 저장
    GetModuleFileNameW(NULL, g_exePath, MAX_PATH_LEN);
    WCHAR *lastSlash = wcsrchr(g_exePath, L'\\');
    if (lastSlash)
        *lastSlash = L'\0';

    // Winsock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        MessageBoxW(NULL, L"Winsock 초기화 실패", L"오류", MB_ICONERROR);
        return 1;
    }

    // 설정 로드
    LoadConfig();

    // 윈도우 클래스 등록
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"PingMonitorWebView";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(NULL, L"윈도우 클래스 등록 실패", L"오류", MB_ICONERROR);
        WSACleanup();
        return 1;
    }

    // 메인 윈도우 생성
    g_hMainWnd = CreateWindowExW(
        0,
        L"PingMonitorWebView",
        L"Ping Monitor - Network Graph",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1, 1, // 최소 크기
        NULL, NULL, hInstance, NULL);

    if (!g_hMainWnd)
    {
        MessageBoxW(NULL, L"윈도우 생성 실패", L"오류", MB_ICONERROR);
        WSACleanup();
        return 1;
    }

    ShowWindow(g_hMainWnd, SW_HIDE); // 창 숨김
    UpdateWindow(g_hMainWnd);

    // 트레이 아이콘 초기화
    InitTrayIcon(g_hMainWnd);

    // HTTP 서버 시작
    if (!StartHttpServer(HTTP_PORT, g_exePath))
    {
        MessageBoxW(NULL,
                    L"HTTP 서버 시작 실패!\n\n"
                    L"포트 8080이 이미 사용 중일 수 있습니다.",
                    L"오류", MB_ICONERROR);
        RemoveTrayIcon();
        WSACleanup();
        return 1;
    }

    // 초기 데이터 파일 생성
    SendDataToWebView();

    // 자동 시작
    StartMonitoring();

    // 브라우저 자동 열기
    OpenBrowser();

    // 메시지 루프
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    WSACleanup();
    return (int)msg.wParam;
}