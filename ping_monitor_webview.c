// ping_monitor_webview.c - 알림 기능 + 포트 변경 추가 버전 (v2.5)
// Windows 네트워크 핑 모니터링 + HTTP 서버 + 트레이 아이콘 + 알림 + 포트 변경

#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")

// HTTP 서버 헤더
#include "http_server.h"

// 외부에서 접근 가능한 종료 플래그
extern HWND g_mainHwnd;

// 상수 정의
#define MAX_IP_COUNT 50
#define MAX_HISTORY 60
#define PING_TIMEOUT 2000
#define JSON_FILE L"ping_data.json"
#define CONFIG_FILE L"ping_config.ini"
#define INT_CONFIG_FILE L"int_config.ini"

// 윈도우 메시지
#define WM_TRAYICON (WM_USER + 1)

// 트레이 아이콘 메뉴 ID
#define ID_TRAY_ICON 1001
#define ID_TRAY_EXIT 1002
#define ID_TRAY_START 1003
#define ID_TRAY_BROWSER 1004
#define ID_TRAY_NOTIFICATIONS 1005
#define ID_TRAY_CHANGE_PORT 1006

// HTTP 서버 설정
#define HTTP_PORT 8080
#define HTTP_PORT_MIN 8000
#define HTTP_PORT_MAX 9000

// 알림 설정 기본값
#define DEFAULT_NOTIFICATION_ENABLED TRUE
#define DEFAULT_NOTIFICATION_COOLDOWN 60
#define DEFAULT_NOTIFY_ON_TIMEOUT TRUE
#define DEFAULT_NOTIFY_ON_RECOVERY TRUE
#define DEFAULT_CONSECUTIVE_FAILURES 3

// 구조체 정의
typedef struct
{
    int pingInterval;
    int maxCount;
    BOOL loop;
} TimeSettings;

typedef struct
{
    wchar_t ip[64];
    wchar_t name[128];
    DWORD latency;
    int online;

    // 통계
    int total;
    int success;
    DWORD min;
    DWORD max;
    double avg;

    // 히스토리
    DWORD history[MAX_HISTORY];
    int historyIndex;

    // 알림 관련
    time_t lastNotificationTime;
    int previousOnline;
    int consecutiveFailures;
    int consecutiveSuccesses;
} IPTarget;

// 알림 설정 구조체
typedef struct
{
    BOOL enabled;
    int cooldown;
    BOOL notifyOnTimeout;
    BOOL notifyOnRecovery;
    int consecutiveFailuresThreshold;
} NotificationSettings;

// 전역 변수
static IPTarget g_targets[MAX_IP_COUNT];
static int g_targetCount = 0;
static TimeSettings g_timeSettings = {1000, 0, TRUE};
static BOOL g_isRunning = FALSE;
static HWND g_hwnd = NULL;
HWND g_mainHwnd = NULL; // HTTP 서버에서 접근 가능
static NOTIFYICONDATAW g_nid = {0};
static wchar_t g_exePath[MAX_PATH] = {0};
static int g_currentPort = HTTP_PORT;

// 알림 설정 전역 변수
static NotificationSettings g_notifSettings = {
    DEFAULT_NOTIFICATION_ENABLED,
    DEFAULT_NOTIFICATION_COOLDOWN,
    DEFAULT_NOTIFY_ON_TIMEOUT,
    DEFAULT_NOTIFY_ON_RECOVERY,
    DEFAULT_CONSECUTIVE_FAILURES};

// 알림 로그 파일 잠금용
static CRITICAL_SECTION g_logLock;

// 함수 선언
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void LoadConfigFromFile(const wchar_t *configFile);
void LoadConfig(void);
void InitTrayIcon(HWND hwnd);
void RemoveTrayIcon(void);
void ShowTrayMenu(HWND hwnd);
BOOL DoPing(const wchar_t *ip, DWORD *latency);
void UpdateTarget(IPTarget *target, BOOL success, DWORD latency);
void SendDataToWebView(void);
void StartMonitoring(void);
void StopMonitoring(void);
DWORD WINAPI MonitoringThread(LPVOID lpParam);
void OpenBrowser(const wchar_t *url);
void KillPreviousInstance(void);

// 알림 관련 함수
void LoadNotificationSettings(void);
void ShowBalloonNotification(const wchar_t *title, const wchar_t *message, DWORD infoFlags);
void CheckAndNotify(IPTarget *target);
void SaveNotificationLog(const wchar_t *type, const wchar_t *name, const wchar_t *ip, const wchar_t *timeStr);

// 포트 관련 함수
BOOL IsPortAvailable(int port);
int FindAvailablePort(int startPort, int endPort);
void ChangeServerPort(HWND hwnd);
BOOL RestartServer(int newPort);

// 포트 사용 가능 여부 확인
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

// 사용 가능한 포트 찾기
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

// 서버 재시작
BOOL RestartServer(int newPort)
{
    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    // 기존 서버 중지
    StopHttpServer();
    Sleep(500);

    // 새 포트로 서버 시작
    if (StartHttpServer(newPort, exeDir))
    {
        g_currentPort = newPort;
        wprintf(L"서버 포트 변경: %d\n", newPort);
        return TRUE;
    }

    return FALSE;
}

// 포트 입력 다이얼로그 프로시저
INT_PTR CALLBACK PortDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int *pNewPort = NULL;

    switch (message)
    {
    case WM_INITDIALOG:
    {
        pNewPort = (int *)lParam;

        // 사용 가능한 포트 찾기
        int availablePort = FindAvailablePort(HTTP_PORT_MIN, HTTP_PORT_MAX);

        // 현재 포트 정보 표시
        wchar_t infoText[512];
        if (availablePort != -1)
        {
            swprintf(infoText, 512,
                     L"현재 포트: %d\n\n권장 포트: %d (사용 가능)\n\n새 포트 번호를 입력하세요 (%d-%d):",
                     g_currentPort, availablePort, HTTP_PORT_MIN, HTTP_PORT_MAX);
        }
        else
        {
            swprintf(infoText, 512,
                     L"현재 포트: %d\n\n경고: 사용 가능한 포트를 찾을 수 없습니다.\n\n새 포트 번호를 입력하세요 (%d-%d):",
                     g_currentPort, HTTP_PORT_MIN, HTTP_PORT_MAX);
        }
        SetDlgItemTextW(hDlg, 1001, infoText);

        // 권장 포트 또는 현재 포트를 입력 필드에 설정
        wchar_t portText[16];
        swprintf(portText, 16, L"%d", availablePort != -1 ? availablePort : g_currentPort);
        SetDlgItemTextW(hDlg, 1002, portText);

        // 입력 필드에 포커스
        SetFocus(GetDlgItem(hDlg, 1002));
        SendDlgItemMessageW(hDlg, 1002, EM_SETSEL, 0, -1);

        return FALSE;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            wchar_t buffer[16];
            GetDlgItemTextW(hDlg, 1002, buffer, 16);
            int newPort = _wtoi(buffer);

            if (newPort >= HTTP_PORT_MIN && newPort <= HTTP_PORT_MAX)
            {
                *pNewPort = newPort;
                EndDialog(hDlg, IDOK);
            }
            else
            {
                wchar_t errorMsg[256];
                swprintf(errorMsg, 256, L"포트는 %d부터 %d 사이여야 합니다.", HTTP_PORT_MIN, HTTP_PORT_MAX);
                MessageBoxW(hDlg, errorMsg, L"입력 오류", MB_OK | MB_ICONWARNING);
            }
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// 포트 입력 다이얼로그 생성
HWND CreatePortDialog(HWND hwndParent, int *pNewPort)
{
// 다이얼로그 템플릿을 메모리에 생성
#pragma pack(push, 1)
    struct
    {
        DLGTEMPLATE dlg;
        WORD menu;
        WORD windowClass;
        WCHAR title[32];
        WORD pointSize;
        WCHAR font[32];
    } dlgTemplate;
#pragma pack(pop)

    memset(&dlgTemplate, 0, sizeof(dlgTemplate));

    dlgTemplate.dlg.style = DS_SETFONT | DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    dlgTemplate.dlg.dwExtendedStyle = 0;
    dlgTemplate.dlg.cdit = 3;
    dlgTemplate.dlg.x = 0;
    dlgTemplate.dlg.y = 0;
    dlgTemplate.dlg.cx = 280;
    dlgTemplate.dlg.cy = 140;
    dlgTemplate.menu = 0;
    dlgTemplate.windowClass = 0;
    wcscpy(dlgTemplate.title, L"포트 변경");
    dlgTemplate.pointSize = 9;
    wcscpy(dlgTemplate.font, L"Malgun Gothic");

    // 다이얼로그 표시
    return (HWND)DialogBoxIndirectParamW(GetModuleHandle(NULL), &dlgTemplate.dlg, hwndParent, PortDialogProc, (LPARAM)pNewPort);
}

// 포트 변경 다이얼로그
void ChangeServerPort(HWND hwnd)
{
    // 간단한 입력 창 생성
    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"#32770", // 다이얼로그 클래스
        L"포트 변경",
        WS_VISIBLE | WS_SYSMENU | WS_CAPTION | DS_MODALFRAME | DS_CENTER,
        0, 0, 350, 230,
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    if (!hDlg)
        return;

    // 화면 중앙 배치
    RECT rcDlg, rcOwner;
    GetWindowRect(hDlg, &rcDlg);
    GetWindowRect(GetDesktopWindow(), &rcOwner);
    SetWindowPos(hDlg, HWND_TOP,
                 (rcOwner.right - rcDlg.right + rcDlg.left) / 2,
                 (rcOwner.bottom - rcDlg.bottom + rcDlg.top) / 2,
                 0, 0, SWP_NOSIZE);

    // Static 텍스트 (안내문)
    int availablePort = FindAvailablePort(HTTP_PORT_MIN, HTTP_PORT_MAX);
    wchar_t infoText[512];
    if (availablePort != -1)
    {
        swprintf(infoText, 512,
                 L"현재 포트: %d\n\n"
                 L"권장 포트: %d (사용 가능)\n\n"
                 L"새 포트 번호를 입력하세요 (%d-%d):",
                 g_currentPort, availablePort, HTTP_PORT_MIN, HTTP_PORT_MAX);
    }
    else
    {
        swprintf(infoText, 512,
                 L"현재 포트: %d\n\n"
                 L"경고: 사용 가능한 포트를 찾을 수 없습니다.\n\n"
                 L"새 포트 번호를 입력하세요 (%d-%d):",
                 g_currentPort, HTTP_PORT_MIN, HTTP_PORT_MAX);
    }

    HWND hStatic = CreateWindowW(
        L"STATIC", infoText,
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, 20, 300, 80,
        hDlg, (HMENU)1001, GetModuleHandle(NULL), NULL);

    // Edit 컨트롤 (포트 입력)
    wchar_t portText[16];
    swprintf(portText, 16, L"%d", availablePort != -1 ? availablePort : g_currentPort);

    HWND hEdit = CreateWindowW(
        L"EDIT", portText,
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER,
        20, 110, 100, 25,
        hDlg, (HMENU)1002, GetModuleHandle(NULL), NULL);

    // 확인 버튼
    HWND hOK = CreateWindowW(
        L"BUTTON", L"확인",
        WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        140, 110, 80, 25,
        hDlg, (HMENU)IDOK, GetModuleHandle(NULL), NULL);

    // 취소 버튼
    HWND hCancel = CreateWindowW(
        L"BUTTON", L"취소",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        230, 110, 80, 25,
        hDlg, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);

    // 폰트 설정
    HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Malgun Gothic");
    SendMessage(hStatic, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hOK, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hCancel, WM_SETFONT, (WPARAM)hFont, TRUE);

    // 포커스 설정
    SetFocus(hEdit);
    SendMessage(hEdit, EM_SETSEL, 0, -1);

    // 메시지 루프
    MSG msg;
    BOOL result = FALSE;
    int newPort = 0;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.hwnd == hDlg || IsChild(hDlg, msg.hwnd))
        {
            if (msg.message == WM_COMMAND)
            {
                if (LOWORD(msg.wParam) == IDOK)
                {
                    wchar_t buffer[16];
                    GetWindowTextW(hEdit, buffer, 16);
                    newPort = _wtoi(buffer);

                    if (newPort >= HTTP_PORT_MIN && newPort <= HTTP_PORT_MAX)
                    {
                        if (IsPortAvailable(newPort) || newPort == g_currentPort)
                        {
                            result = TRUE;
                            DestroyWindow(hDlg);
                            break;
                        }
                        else
                        {
                            wchar_t errorMsg[256];
                            swprintf(errorMsg, 256, L"포트 %d는 이미 사용 중입니다.\n\n다른 포트를 입력하세요.", newPort);
                            MessageBoxW(hDlg, errorMsg, L"포트 사용 중", MB_OK | MB_ICONWARNING);
                        }
                    }
                    else
                    {
                        wchar_t errorMsg[256];
                        swprintf(errorMsg, 256, L"포트는 %d부터 %d 사이여야 합니다.", HTTP_PORT_MIN, HTTP_PORT_MAX);
                        MessageBoxW(hDlg, errorMsg, L"입력 오류", MB_OK | MB_ICONWARNING);
                    }
                }
                else if (LOWORD(msg.wParam) == IDCANCEL)
                {
                    DestroyWindow(hDlg);
                    break;
                }
            }
            else if (msg.message == WM_CLOSE)
            {
                DestroyWindow(hDlg);
                break;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DeleteObject(hFont);

    // 결과 처리
    if (result && newPort > 0)
    {
        if (RestartServer(newPort))
        {
            wchar_t successMsg[256];
            swprintf(successMsg, 256,
                     L"포트가 %d로 변경되었습니다.\n\n"
                     L"새 주소: http://localhost:%d",
                     newPort, newPort);
            MessageBoxW(hwnd, successMsg, L"성공", MB_OK | MB_ICONINFORMATION);

            // 브라우저 열기
            wchar_t url[256];
            swprintf(url, 256, L"http://localhost:%d/graph.html", newPort);
            OpenBrowser(url);
        }
        else
        {
            MessageBoxW(hwnd, L"포트 변경 실패. 다른 포트를 시도해주세요.", L"오류", MB_OK | MB_ICONERROR);
        }
    }
}

// 설정 파일 로드
void LoadConfigFromFile(const wchar_t *configFile)
{
    wchar_t configPath[MAX_PATH];
    wchar_t exeDir[MAX_PATH];

    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    swprintf(configPath, MAX_PATH, L"%s\\%s", exeDir, configFile);

    FILE *file = _wfopen(configPath, L"r, ccs=UTF-8");
    if (!file)
    {
        file = _wfopen(configPath, L"r");
    }

    if (!file)
    {
        wprintf(L"설정 파일을 열 수 없습니다: %s (무시)\n", configPath);
        return;
    }

    wprintf(L"설정 파일 읽는 중: %s\n", configFile);

    wchar_t line[512];
    BOOL inSettingsSection = FALSE;

    while (fgetws(line, 512, file) && g_targetCount < MAX_IP_COUNT)
    {
        line[wcscspn(line, L"\r\n")] = 0;

        if (line[0] == 0 || line[0] == L'#' || line[0] == L';')
        {
            continue;
        }

        if (wcsstr(line, L"[Settings]") || wcsstr(line, L"[SETTINGS]"))
        {
            inSettingsSection = TRUE;
            continue;
        }

        if (line[0] == L'[')
        {
            inSettingsSection = FALSE;
            continue;
        }

        if (inSettingsSection)
        {
            wchar_t key[128], value[128];
            if (swscanf(line, L"%127[^=]=%127s", key, value) == 2)
            {
                wchar_t *k = key;
                while (*k == L' ' || *k == L'\t')
                    k++;
                wchar_t *kend = k + wcslen(k) - 1;
                while (kend > k && (*kend == L' ' || *kend == L'\t'))
                    *kend-- = 0;

                if (_wcsicmp(k, L"NotificationsEnabled") == 0)
                {
                    g_notifSettings.enabled = (_wtoi(value) != 0);
                }
                else if (_wcsicmp(k, L"NotificationCooldown") == 0)
                {
                    g_notifSettings.cooldown = _wtoi(value);
                    if (g_notifSettings.cooldown < 0)
                        g_notifSettings.cooldown = 60;
                }
                else if (_wcsicmp(k, L"NotifyOnTimeout") == 0)
                {
                    g_notifSettings.notifyOnTimeout = (_wtoi(value) != 0);
                }
                else if (_wcsicmp(k, L"NotifyOnRecovery") == 0)
                {
                    g_notifSettings.notifyOnRecovery = (_wtoi(value) != 0);
                }
                else if (_wcsicmp(k, L"ConsecutiveFailures") == 0)
                {
                    g_notifSettings.consecutiveFailuresThreshold = _wtoi(value);
                    if (g_notifSettings.consecutiveFailuresThreshold < 1)
                    {
                        g_notifSettings.consecutiveFailuresThreshold = 3;
                    }
                }
                continue;
            }
            else
            {
                if (wcschr(line, L','))
                {
                    inSettingsSection = FALSE;
                }
                else
                {
                    continue;
                }
            }
        }

        wchar_t *comma = wcschr(line, L',');
        if (comma)
        {
            *comma = 0;
            wchar_t *ip = line;
            wchar_t *name = comma + 1;

            while (*ip == L' ' || *ip == L'\t')
                ip++;
            while (*name == L' ' || *name == L'\t')
                name++;

            wcscpy(g_targets[g_targetCount].ip, ip);
            wcscpy(g_targets[g_targetCount].name, name);

            g_targets[g_targetCount].latency = 0;
            g_targets[g_targetCount].online = 0;
            g_targets[g_targetCount].total = 0;
            g_targets[g_targetCount].success = 0;
            g_targets[g_targetCount].min = 0;
            g_targets[g_targetCount].max = 0;
            g_targets[g_targetCount].avg = 0.0;
            g_targets[g_targetCount].historyIndex = 0;

            g_targets[g_targetCount].lastNotificationTime = 0;
            g_targets[g_targetCount].previousOnline = -1;
            g_targets[g_targetCount].consecutiveFailures = 0;
            g_targets[g_targetCount].consecutiveSuccesses = 0;

            memset(g_targets[g_targetCount].history, 0, sizeof(g_targets[g_targetCount].history));

            g_targetCount++;
        }
    }

    fclose(file);
    wprintf(L"  %s에서 타겟 로드 완료\n", configFile);
}

void LoadConfig(void)
{
    g_targetCount = 0;

    wprintf(L"==========================================\n");
    wprintf(L"설정 파일 로딩 시작\n");
    wprintf(L"==========================================\n");
    LoadConfigFromFile(CONFIG_FILE);

    wprintf(L"------------------------------------------\n");
    LoadConfigFromFile(INT_CONFIG_FILE);

    wprintf(L"==========================================\n");
    wprintf(L"설정 로드 완료: 총 %d개 타겟\n", g_targetCount);
    wprintf(L"알림 설정: %s (쿨다운: %d초, 연속실패: %d회)\n",
            g_notifSettings.enabled ? L"활성화" : L"비활성화",
            g_notifSettings.cooldown,
            g_notifSettings.consecutiveFailuresThreshold);
    wprintf(L"==========================================\n");
}

// 트레이 아이콘 풍선 알림 표시
void ShowBalloonNotification(const wchar_t *title, const wchar_t *message, DWORD infoFlags)
{
    if (!g_notifSettings.enabled)
        return;

    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = g_hwnd;
    nid.uID = ID_TRAY_ICON;
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = infoFlags;
    nid.uTimeout = 5000;

    wcsncpy(nid.szInfoTitle, title, ARRAYSIZE(nid.szInfoTitle) - 1);
    nid.szInfoTitle[ARRAYSIZE(nid.szInfoTitle) - 1] = 0;

    wcsncpy(nid.szInfo, message, ARRAYSIZE(nid.szInfo) - 1);
    nid.szInfo[ARRAYSIZE(nid.szInfo) - 1] = 0;

    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

// 타임아웃 및 복구 감지 후 알림
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

        wprintf(L"[알림 %s] 타임아웃: %s (%s)\n", timeStr, target->name, target->ip);

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

        wprintf(L"[알림 %s] 복구: %s (%s)\n", timeStr, target->name, target->ip);

        SaveNotificationLog(L"recovery", target->name, target->ip, timeStr);
    }

    target->previousOnline = target->online;
}

// 알림 로그 JSON 파일에 저장
void SaveNotificationLog(const wchar_t *type, const wchar_t *name, const wchar_t *ip, const wchar_t *timeStr)
{
    EnterCriticalSection(&g_logLock);

    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    wchar_t logPath[MAX_PATH];
    swprintf(logPath, MAX_PATH, L"%s\\notification_log.json", exeDir);

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
        swprintf(tmpPath, MAX_PATH, L"%s\\notification_log.json.tmp", exeDir);

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

// ICMP 핑 실행
BOOL DoPing(const wchar_t *ip, DWORD *latency)
{
    HANDLE hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    char ipStr[64];
    WideCharToMultiByte(CP_UTF8, 0, ip, -1, ipStr, sizeof(ipStr), NULL, NULL);

    unsigned long ipAddr = inet_addr(ipStr);
    if (ipAddr == INADDR_NONE)
    {
        IcmpCloseHandle(hIcmpFile);
        return FALSE;
    }

    char sendData[32] = "PingMonitorData";
    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData);
    LPVOID replyBuffer = malloc(replySize);

    if (!replyBuffer)
    {
        IcmpCloseHandle(hIcmpFile);
        return FALSE;
    }

    DWORD result = IcmpSendEcho(
        hIcmpFile,
        ipAddr,
        sendData,
        sizeof(sendData),
        NULL,
        replyBuffer,
        replySize,
        PING_TIMEOUT);

    BOOL success = FALSE;
    if (result != 0)
    {
        PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)replyBuffer;
        if (pEchoReply->Status == IP_SUCCESS)
        {
            *latency = pEchoReply->RoundTripTime;
            success = TRUE;
        }
    }

    free(replyBuffer);
    IcmpCloseHandle(hIcmpFile);

    return success;
}

// 타겟 통계 업데이트
void UpdateTarget(IPTarget *target, BOOL success, DWORD latency)
{
    target->total++;

    if (success)
    {
        target->success++;
        target->latency = latency;
        target->online = 1;

        target->consecutiveSuccesses++;
        target->consecutiveFailures = 0;

        if (target->min == 0 || latency < target->min)
            target->min = latency;
        if (latency > target->max)
            target->max = latency;

        double total_latency = target->avg * (target->success - 1) + latency;
        target->avg = total_latency / target->success;
    }
    else
    {
        target->latency = 0;
        target->online = 0;

        target->consecutiveFailures++;
        target->consecutiveSuccesses = 0;
    }

    target->history[target->historyIndex] = target->latency;
    target->historyIndex = (target->historyIndex + 1) % MAX_HISTORY;

    CheckAndNotify(target);
}

// JSON 데이터 생성
void BuildJsonData(wchar_t *buffer, size_t bufferSize)
{
    int offset = 0;

    offset += swprintf(buffer + offset, bufferSize - offset,
                       L"{\n  \"running\": %s,\n  \"elapsed\": %d,\n  \"total\": %d,\n  \"loop\": %s,\n",
                       g_isRunning ? L"true" : L"false",
                       g_targetCount > 0 ? g_targets[0].total : 0,
                       g_targetCount > 0 ? g_targets[0].total : 0,
                       g_timeSettings.loop ? L"true" : L"false");

    offset += swprintf(buffer + offset, bufferSize - offset, L"  \"targets\": [\n");

    for (int i = 0; i < g_targetCount; i++)
    {
        IPTarget *t = &g_targets[i];

        offset += swprintf(buffer + offset, bufferSize - offset,
                           L"    {\n"
                           L"      \"ip\": \"%s\",\n"
                           L"      \"name\": \"%s\",\n"
                           L"      \"latency\": %lu,\n"
                           L"      \"online\": %d,\n"
                           L"      \"total\": %d,\n"
                           L"      \"success\": %d,\n"
                           L"      \"min\": %lu,\n"
                           L"      \"max\": %lu,\n"
                           L"      \"avg\": %.2f,\n",
                           t->ip, t->name, t->latency, t->online,
                           t->total, t->success, t->min, t->max, t->avg);

        offset += swprintf(buffer + offset, bufferSize - offset, L"      \"history\": [");

        for (int j = 0; j < MAX_HISTORY; j++)
        {
            int idx = (t->historyIndex + j) % MAX_HISTORY;
            offset += swprintf(buffer + offset, bufferSize - offset, L"%lu", t->history[idx]);
            if (j < MAX_HISTORY - 1)
            {
                offset += swprintf(buffer + offset, bufferSize - offset, L",");
            }
        }

        offset += swprintf(buffer + offset, bufferSize - offset, L"]\n    }");

        if (i < g_targetCount - 1)
        {
            offset += swprintf(buffer + offset, bufferSize - offset, L",\n");
        }
        else
        {
            offset += swprintf(buffer + offset, bufferSize - offset, L"\n");
        }
    }

    offset += swprintf(buffer + offset, bufferSize - offset, L"  ]\n}\n");
}

// JSON 파일 저장 (원자적 쓰기)
void SendDataToWebView(void)
{
    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    wchar_t jsonPath[MAX_PATH];
    wchar_t tmpPath[MAX_PATH];
    swprintf(jsonPath, MAX_PATH, L"%s\\%s", exeDir, JSON_FILE);
    swprintf(tmpPath, MAX_PATH, L"%s\\%s.tmp", exeDir, JSON_FILE);

    size_t bufferSize = 10 * 1024 * 1024;
    wchar_t *jsonData = (wchar_t *)malloc(bufferSize);
    if (!jsonData)
        return;

    BuildJsonData(jsonData, bufferSize / sizeof(wchar_t));

    FILE *tmpFile = _wfopen(tmpPath, L"w, ccs=UTF-8");
    if (tmpFile)
    {
        fwprintf(tmpFile, L"%s", jsonData);
        fclose(tmpFile);

        DeleteFileW(jsonPath);
        MoveFileW(tmpPath, jsonPath);
    }

    free(jsonData);
}

// 모니터링 스레드
DWORD WINAPI MonitoringThread(LPVOID lpParam)
{
    while (g_isRunning)
    {
        for (int i = 0; i < g_targetCount; i++)
        {
            DWORD latency = 0;
            BOOL success = DoPing(g_targets[i].ip, &latency);
            UpdateTarget(&g_targets[i], success, latency);
        }

        SendDataToWebView();

        Sleep(g_timeSettings.pingInterval);
    }

    return 0;
}

// 모니터링 시작
void StartMonitoring(void)
{
    if (g_isRunning)
        return;

    g_isRunning = TRUE;
    CreateThread(NULL, 0, MonitoringThread, NULL, 0, NULL);

    wprintf(L"모니터링 시작\n");

    wcscpy(g_nid.szTip, L"Ping Monitor - 실행 중");
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

// 모니터링 중지
void StopMonitoring(void)
{
    if (!g_isRunning)
        return;

    g_isRunning = FALSE;
    wprintf(L"모니터링 중지\n");

    wcscpy(g_nid.szTip, L"Ping Monitor - 중지됨");
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

// 트레이 아이콘 초기화
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

// 트레이 아이콘 제거
void RemoveTrayIcon(void)
{
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

// 트레이 메뉴 표시
void ShowTrayMenu(HWND hwnd)
{
    HMENU hMenu = CreatePopupMenu();

    AppendMenuW(hMenu, MF_STRING, ID_TRAY_START, g_isRunning ? L"일시정지" : L"시작");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_BROWSER, L"브라우저 열기");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    UINT checkFlag = g_notifSettings.enabled ? MF_CHECKED : MF_UNCHECKED;
    AppendMenuW(hMenu, MF_STRING | checkFlag, ID_TRAY_NOTIFICATIONS, L"알림 활성화");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_CHANGE_PORT, L"포트 변경...");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"종료");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

// 브라우저 열기
void OpenBrowser(const wchar_t *url)
{
    ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);
}

// 이전 인스턴스 종료
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

// 윈도우 프로시저
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
            swprintf(url, 256, L"http://localhost:%d/graph.html", g_currentPort);
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
            swprintf(url, 256, L"http://localhost:%d/graph.html", g_currentPort);
            OpenBrowser(url);
            break;
        }

        case ID_TRAY_NOTIFICATIONS:
            g_notifSettings.enabled = !g_notifSettings.enabled;
            wprintf(L"알림: %s\n", g_notifSettings.enabled ? L"활성화" : L"비활성화");

            if (g_notifSettings.enabled)
            {
                ShowBalloonNotification(L"알림 활성화",
                                        L"네트워크 타임아웃 및 복구 알림이 활성화되었습니다.",
                                        NIIF_INFO);
            }
            break;

        case ID_TRAY_CHANGE_PORT:
            ChangeServerPort(hwnd);
            break;

        case ID_TRAY_EXIT:
            PostQuitMessage(0);
            break;
        }
        break;

    case WM_DESTROY:
        RemoveTrayIcon();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 메인 함수
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        MessageBoxW(NULL, L"Winsock 초기화 실패", L"오류", MB_OK | MB_ICONERROR);
        return 1;
    }

    InitializeCriticalSection(&g_logLock);

    KillPreviousInstance();

    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    LoadConfig();

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
                     L"포트 %d-%d 범위에서 사용 가능한 포트를 찾을 수 없습니다.\n\n"
                     L"다른 프로그램이 해당 포트를 사용 중일 수 있습니다.",
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

    g_mainHwnd = g_hwnd; // HTTP 서버에서 접근 가능하도록 설정

    ShowWindow(g_hwnd, SW_HIDE);

    StartMonitoring();

    wchar_t url[256];
    swprintf(url, 256, L"http://localhost:%d/graph.html", g_currentPort);
    OpenBrowser(url);

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