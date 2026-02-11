/**
 * Notification Module Implementation (v2.7)
 * 상태 전환 기반 알림 시스템
 * - 온라인→오프라인: 팝업 + 기록
 * - 오프라인 지속: 기록만 (팝업 X)
 * - 오프라인→온라인: 팝업 + 기록
 * - 여러 IP 동시 알림 지원
 */

#include "notification.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <shlwapi.h>

// ============================================================================
// Constants
// ============================================================================

#define WM_SHOW_NOTIFICATION (WM_USER + 100)
#define MAX_NOTIFICATION_WINDOWS 10
#define NOTIFICATION_WIDTH 300
#define NOTIFICATION_HEIGHT 80
#define NOTIFICATION_MARGIN 10
#define NOTIFICATION_DURATION 5000
#define FADE_STEPS 20
#define FADE_INTERVAL 25

// ============================================================================
// Structures
// ============================================================================

typedef struct
{
    HWND hwnd;
    BOOL isActive;
    int fadeStep;
    UINT_PTR fadeTimerId;
    UINT_PTR closeTimerId;
} NotificationWindow;

// NotificationRequest는 notification.h에서 정의됨

// ============================================================================
// Static Variables
// ============================================================================

static NotificationWindow s_windows[MAX_NOTIFICATION_WINDOWS] = {0};
static HFONT s_titleFont = NULL;
static HFONT s_messageFont = NULL;
static BOOL s_initialized = FALSE;
static const wchar_t *NOTIFICATION_CLASS = L"PingMonitorNotification";

// 동시 오프라인 감지용
#define BATCH_WINDOW_MS 2000 // 2초 내 발생한 알림을 배치로 간주
#define MAX_POPUP_COUNT 10   // 이 개수 초과 시 팝업 억제

static time_t s_batchStartTime = 0;
static int s_batchOfflineCount = 0;
static BOOL s_batchPopupSuppressed = FALSE;

// ============================================================================
// Forward Declarations
// ============================================================================

static LRESULT CALLBACK NotificationWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void RegisterNotificationClass(void);
static int FindAvailableSlot(void);
static void PositionNotification(HWND hwnd, int slot);
static void StartFadeIn(int slot);
static void StartFadeOut(int slot);
static void WriteNotificationLog(const wchar_t *type, const wchar_t *name, const wchar_t *ip);
static void CloseAllNotificationWindows(void);

// Timer callbacks
static void CALLBACK FadeInTimerProc(HWND hwnd, UINT msg, UINT_PTR idTimer, DWORD dwTime);
static void CALLBACK CloseTimerProc(HWND hwnd, UINT msg, UINT_PTR idTimer, DWORD dwTime);
static void CALLBACK FadeOutTimerProc(HWND hwnd, UINT msg, UINT_PTR idTimer, DWORD dwTime);

// ============================================================================
// Initialization
// ============================================================================

void InitNotificationSystem(void)
{
    if (s_initialized)
        return;

    RegisterNotificationClass();

    // 폰트 생성 (한 번만)
    s_titleFont = CreateFontW(
        14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"맑은 고딕");

    s_messageFont = CreateFontW(
        12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"맑은 고딕");

    s_initialized = TRUE;
    wprintf(L"[알림 시스템] 초기화 완료\n");
}

void CleanupNotificationSystem(void)
{
    // 모든 알림창 닫기
    for (int i = 0; i < MAX_NOTIFICATION_WINDOWS; i++)
    {
        if (s_windows[i].hwnd && IsWindow(s_windows[i].hwnd))
        {
            DestroyWindow(s_windows[i].hwnd);
        }
        s_windows[i].hwnd = NULL;
        s_windows[i].isActive = FALSE;
    }

    // 폰트 해제
    if (s_titleFont)
    {
        DeleteObject(s_titleFont);
        s_titleFont = NULL;
    }
    if (s_messageFont)
    {
        DeleteObject(s_messageFont);
        s_messageFont = NULL;
    }

    // 배치 상태 리셋
    s_batchStartTime = 0;
    s_batchOfflineCount = 0;
    s_batchPopupSuppressed = FALSE;

    s_initialized = FALSE;
}

// ============================================================================
// Window Class Registration
// ============================================================================

static void RegisterNotificationClass(void)
{
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = NotificationWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = NOTIFICATION_CLASS;
    wc.hbrBackground = NULL; // 직접 그리기
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);
}

// ============================================================================
// Core Notification Functions
// ============================================================================

/**
 * 상태 전환 확인 및 알림 처리
 * - previousOnline → online 변화 감지
 * - 새로운 오프라인 전환: 팝업 + 기록
 * - 오프라인 지속: 기록만
 * - 복구: 팝업 + 기록
 * - 동시 10개 이상 오프라인: 팝업 억제, 기록만
 */
void CheckAndNotify(IPTarget *target)
{
    if (!g_notifSettings.enabled)
        return;

    time_t now = time(NULL);
    BOOL showPopup = FALSE;
    BOOL isNewOffline = FALSE;
    BOOL isRecovery = FALSE;

    // 첫 번째 핑 결과 (total == 1): 기준점 설정
    // 초기 previousOnline은 1로 가정 (온라인에서 시작)
    if (target->total == 1)
    {
        // 첫 번째 결과가 오프라인이면 즉시 알림 (consecutiveFailures 무시)
        if (target->online == 0)
        {
            isNewOffline = TRUE;
            showPopup = g_notifSettings.notifyOnTimeout;
            wprintf(L"[알림] %s (%s) - 최초 오프라인 감지 (total=%d)\n", target->name, target->ip, target->total);

            // 배치 카운트 시작
            if (s_batchStartTime == 0)
            {
                s_batchStartTime = now;
            }
            s_batchOfflineCount++;

            // 10개 초과 체크
            if (s_batchOfflineCount > MAX_POPUP_COUNT)
            {
                if (!s_batchPopupSuppressed)
                {
                    wprintf(L"[알림] 동시 오프라인 %d개 초과 - 팝업 억제, 기록만\n", MAX_POPUP_COUNT);
                    s_batchPopupSuppressed = TRUE;
                    CloseAllNotificationWindows();
                }
                showPopup = FALSE;
            }

            // 쿨다운 체크
            if (showPopup && (now - target->lastNotificationTime) < g_notifSettings.cooldown)
            {
                showPopup = FALSE;
            }

            // 팝업 표시
            if (showPopup)
            {
                target->lastNotificationTime = now;
                ShowCustomNotification(L"⚠️ 연결 끊김", target->name, target->ip, TRUE);
            }

            // 로그 기록 (항상)
            WriteNotificationLog(L"timeout", target->name, target->ip);
        }
        // 첫 번째 결과가 온라인이면 정상 - 알림 없음
        target->previousOnline = target->online;
        return;
    }

    // 배치 윈도우 리셋 체크 (2초 경과 시)
    if (s_batchStartTime > 0 && difftime(now, s_batchStartTime) > (BATCH_WINDOW_MS / 1000.0))
    {
        // 배치 종료 - 리셋
        if (s_batchPopupSuppressed)
        {
            wprintf(L"[알림] 대량 오프라인 배치 종료 (총 %d개, 팝업 억제됨)\n", s_batchOfflineCount);
        }
        s_batchStartTime = 0;
        s_batchOfflineCount = 0;
        s_batchPopupSuppressed = FALSE;
    }

    // 상태 전환 감지 (2번째 핑부터)
    if (target->previousOnline == 1 && target->online == 0)
    {
        // 온라인 → 오프라인 (새로운 장애)
        if (target->consecutiveFailures >= g_notifSettings.consecutiveFailuresThreshold)
        {
            isNewOffline = TRUE;

            // 배치 카운트 증가
            if (s_batchStartTime == 0)
            {
                s_batchStartTime = now;
            }
            s_batchOfflineCount++;

            // 10개 이상이면 팝업 억제
            if (s_batchOfflineCount > MAX_POPUP_COUNT)
            {
                if (!s_batchPopupSuppressed)
                {
                    wprintf(L"[알림] 동시 오프라인 %d개 초과 - 팝업 억제, 기록만\n", MAX_POPUP_COUNT);
                    s_batchPopupSuppressed = TRUE;

                    // 기존 팝업들 모두 닫기
                    CloseAllNotificationWindows();
                }
                showPopup = FALSE;
            }
            else
            {
                showPopup = g_notifSettings.notifyOnTimeout && !s_batchPopupSuppressed;
            }

            wprintf(L"[알림] %s (%s) - 새로운 오프라인 전환 (배치 %d/%d)\n",
                    target->name, target->ip, s_batchOfflineCount, MAX_POPUP_COUNT);
        }
    }
    else if (target->previousOnline == 0 && target->online == 1)
    {
        // 오프라인 → 온라인 (복구)
        isRecovery = TRUE;
        showPopup = g_notifSettings.notifyOnRecovery && !s_batchPopupSuppressed;
        wprintf(L"[알림] %s (%s) - 복구됨\n", target->name, target->ip);
    }
    else if (target->previousOnline == 0 && target->online == 0)
    {
        // 오프라인 지속 - 팝업 없이 (이미 알림 보냄)
        showPopup = FALSE;
    }

    // 쿨다운 체크 (팝업용)
    if (showPopup && (now - target->lastNotificationTime) < g_notifSettings.cooldown)
    {
        showPopup = FALSE;
    }

    // 팝업 알림 표시
    if (showPopup)
    {
        target->lastNotificationTime = now;

        if (isNewOffline)
        {
            ShowCustomNotification(L"⚠️ 연결 끊김", target->name, target->ip, TRUE);
            WriteNotificationLog(L"timeout", target->name, target->ip);
        }
        else if (isRecovery)
        {
            ShowCustomNotification(L"✅ 연결 복구", target->name, target->ip, FALSE);
            WriteNotificationLog(L"recovery", target->name, target->ip);
        }
    }
    else if (isNewOffline || isRecovery)
    {
        // 팝업은 안 뜨지만 로그는 기록
        if (isNewOffline)
        {
            WriteNotificationLog(L"timeout", target->name, target->ip);
        }
        else if (isRecovery)
        {
            WriteNotificationLog(L"recovery", target->name, target->ip);
        }
    }

    // 상태 업데이트 (다음 비교를 위해)
    target->previousOnline = target->online;
}

/**
 * 커스텀 알림창 표시 (PostMessage 방식 - 스레드 안전)
 */
void ShowCustomNotification(const wchar_t *title, const wchar_t *name, const wchar_t *ip, BOOL isTimeout)
{
    if (!g_notifSettings.enabled || !g_mainHwnd)
        return;

    // 메인 스레드로 메시지 전송
    NotificationRequest *req = (NotificationRequest *)malloc(sizeof(NotificationRequest));
    if (!req)
        return;

    swprintf(req->title, 256, L"%s", title);
    swprintf(req->message, 512, L"%s\n%s", name, ip);
    req->isTimeout = isTimeout;

    PostMessage(g_mainHwnd, WM_SHOW_NOTIFICATION, 0, (LPARAM)req);
}

/**
 * 메인 스레드에서 알림창 실제 생성
 */
void ProcessShowNotification(NotificationRequest *req)
{
    if (!req)
        return;

    int slot = FindAvailableSlot();
    if (slot < 0)
    {
        wprintf(L"[알림] 슬롯 부족 - 알림 생략\n");
        return;
    }

    // 알림창 생성
    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        NOTIFICATION_CLASS,
        NULL,
        WS_POPUP,
        0, 0,
        NOTIFICATION_WIDTH, NOTIFICATION_HEIGHT,
        NULL, NULL, GetModuleHandle(NULL), NULL);

    if (!hwnd)
    {
        wprintf(L"[알림] 창 생성 실패\n");
        return;
    }

    // 창 데이터 저장
    s_windows[slot].hwnd = hwnd;
    s_windows[slot].isActive = TRUE;
    s_windows[slot].fadeStep = 0;

    // 데이터를 창에 연결
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)_wcsdup(req->message));

    // 배경색 설정 (타임아웃: 빨간색 계열, 복구: 녹색 계열)
    // 창에 슬롯 번호 저장
    SetPropW(hwnd, L"Slot", (HANDLE)(LONG_PTR)slot);
    SetPropW(hwnd, L"IsTimeout", (HANDLE)(LONG_PTR)req->isTimeout);

    // 초기 투명도 0
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);

    // 위치 설정
    PositionNotification(hwnd, slot);

    // 표시 및 페이드인 시작
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    StartFadeIn(slot);

    wprintf(L"[알림] 창 생성됨 (슬롯 %d)\n", slot);
}

// ============================================================================
// Helper Functions
// ============================================================================

static int FindAvailableSlot(void)
{
    for (int i = 0; i < MAX_NOTIFICATION_WINDOWS; i++)
    {
        if (!s_windows[i].isActive)
        {
            return i;
        }
    }
    return -1;
}

static void PositionNotification(HWND hwnd, int slot)
{
    // 작업 영역 (트레이 제외)
    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

    int x = workArea.right - NOTIFICATION_WIDTH - NOTIFICATION_MARGIN;
    int y = workArea.bottom - (NOTIFICATION_HEIGHT + NOTIFICATION_MARGIN) * (slot + 1);

    SetWindowPos(hwnd, HWND_TOPMOST, x, y,
                 NOTIFICATION_WIDTH, NOTIFICATION_HEIGHT,
                 SWP_NOACTIVATE);
}

/**
 * 모든 알림창 즉시 닫기 (대량 오프라인 시 사용)
 */
static void CloseAllNotificationWindows(void)
{
    for (int i = 0; i < MAX_NOTIFICATION_WINDOWS; i++)
    {
        if (s_windows[i].isActive && s_windows[i].hwnd && IsWindow(s_windows[i].hwnd))
        {
            // 타이머 정리
            if (s_windows[i].fadeTimerId)
            {
                KillTimer(s_windows[i].hwnd, s_windows[i].fadeTimerId);
                s_windows[i].fadeTimerId = 0;
            }
            if (s_windows[i].closeTimerId)
            {
                KillTimer(s_windows[i].hwnd, s_windows[i].closeTimerId);
                s_windows[i].closeTimerId = 0;
            }

            // 창 파괴
            DestroyWindow(s_windows[i].hwnd);
            s_windows[i].hwnd = NULL;
            s_windows[i].isActive = FALSE;
        }
    }

    wprintf(L"[알림] 모든 팝업창 닫힘 (대량 오프라인 감지)\n");
}

// ============================================================================
// Fade Animation
// ============================================================================

static void CALLBACK FadeInTimerProc(HWND hwnd, UINT msg, UINT_PTR idTimer, DWORD dwTime)
{
    int slot = (int)(LONG_PTR)GetPropW(hwnd, L"Slot");

    s_windows[slot].fadeStep++;
    int alpha = (255 * s_windows[slot].fadeStep) / FADE_STEPS;

    if (alpha >= 255)
    {
        alpha = 255;
        KillTimer(hwnd, idTimer);
        s_windows[slot].fadeTimerId = 0;

        // 자동 닫기 타이머 시작
        s_windows[slot].closeTimerId = SetTimer(hwnd, 2, NOTIFICATION_DURATION, CloseTimerProc);
    }

    SetLayeredWindowAttributes(hwnd, 0, (BYTE)alpha, LWA_ALPHA);
}

static void CALLBACK CloseTimerProc(HWND hwnd, UINT msg, UINT_PTR idTimer, DWORD dwTime)
{
    KillTimer(hwnd, idTimer);
    int slot = (int)(LONG_PTR)GetPropW(hwnd, L"Slot");
    s_windows[slot].closeTimerId = 0;
    StartFadeOut(slot);
}

static void CALLBACK FadeOutTimerProc(HWND hwnd, UINT msg, UINT_PTR idTimer, DWORD dwTime)
{
    int slot = (int)(LONG_PTR)GetPropW(hwnd, L"Slot");

    s_windows[slot].fadeStep--;
    int alpha = (255 * s_windows[slot].fadeStep) / FADE_STEPS;

    if (alpha <= 0)
    {
        KillTimer(hwnd, idTimer);
        s_windows[slot].fadeTimerId = 0;

        // 창 파괴
        DestroyWindow(hwnd);
        s_windows[slot].hwnd = NULL;
        s_windows[slot].isActive = FALSE;

        wprintf(L"[알림] 창 닫힘 (슬롯 %d)\n", slot);
    }
    else
    {
        SetLayeredWindowAttributes(hwnd, 0, (BYTE)alpha, LWA_ALPHA);
    }
}

static void StartFadeIn(int slot)
{
    s_windows[slot].fadeStep = 0;
    s_windows[slot].fadeTimerId = SetTimer(s_windows[slot].hwnd, 1, FADE_INTERVAL, FadeInTimerProc);
}

static void StartFadeOut(int slot)
{
    // 기존 타이머 정리
    if (s_windows[slot].closeTimerId)
    {
        KillTimer(s_windows[slot].hwnd, s_windows[slot].closeTimerId);
        s_windows[slot].closeTimerId = 0;
    }

    s_windows[slot].fadeStep = FADE_STEPS;
    s_windows[slot].fadeTimerId = SetTimer(s_windows[slot].hwnd, 3, FADE_INTERVAL, FadeOutTimerProc);
}

// ============================================================================
// Window Procedure
// ============================================================================

static LRESULT CALLBACK NotificationWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);

        // 배경색 (타임아웃/복구에 따라)
        BOOL isTimeout = (BOOL)(LONG_PTR)GetPropW(hwnd, L"IsTimeout");
        HBRUSH bgBrush = CreateSolidBrush(isTimeout ? RGB(220, 53, 69) : RGB(40, 167, 69));
        FillRect(hdc, &rect, bgBrush);
        DeleteObject(bgBrush);

        // 테두리
        HPEN borderPen = CreatePen(PS_SOLID, 1, isTimeout ? RGB(185, 28, 48) : RGB(30, 126, 52));
        SelectObject(hdc, borderPen);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, 0, 0, rect.right, rect.bottom);
        DeleteObject(borderPen);

        // 텍스트
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));

        wchar_t *message = (wchar_t *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (message)
        {
            SelectObject(hdc, s_titleFont ? s_titleFont : GetStockObject(DEFAULT_GUI_FONT));

            RECT textRect = {10, 10, rect.right - 10, rect.bottom - 10};
            DrawTextW(hdc, message, -1, &textRect, DT_LEFT | DT_WORDBREAK);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        // 클릭 시 즉시 닫기
        int slot = (int)(LONG_PTR)GetPropW(hwnd, L"Slot");
        StartFadeOut(slot);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1; // 배경 지우기 처리 완료

    case WM_DESTROY:
    {
        // 메모리 해제
        wchar_t *message = (wchar_t *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (message)
            free(message);

        RemovePropW(hwnd, L"Slot");
        RemovePropW(hwnd, L"IsTimeout");
        return 0;
    }

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

// ============================================================================
// Log Writing
// ============================================================================

static void WriteNotificationLog(const wchar_t *type, const wchar_t *name, const wchar_t *ip)
{
    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    wchar_t logPath[MAX_PATH];
    swprintf(logPath, MAX_PATH, L"%s\\data\\notification_log.json", exeDir);

    // 시간 문자열
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    wchar_t timeStr[32], dateStr[32];
    swprintf(timeStr, 32, L"%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    swprintf(dateStr, 32, L"%04d-%02d-%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);

    wprintf(L"[로그] 알림 기록 저장: %s - %s (%s) at %s %s\n", type, name, ip, dateStr, timeStr);

    EnterCriticalSection(&g_logLock);

    // 기존 로그 읽기 (텍스트 모드)
    FILE *fp = _wfopen(logPath, L"r");
    char *existingData = NULL;
    long fileSize = 0;

    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        fileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (fileSize > 4) // "[]" 이상
        {
            existingData = (char *)malloc(fileSize + 1);
            if (existingData)
            {
                size_t readLen = fread(existingData, 1, fileSize, fp);
                existingData[readLen] = '\0';
            }
        }
        fclose(fp);
    }

    // 새 로그 작성 (UTF-8)
    fp = _wfopen(logPath, L"w, ccs=UTF-8");
    if (fp)
    {
        // 새 항목 (맨 앞에 추가)
        fwprintf(fp, L"[\n");
        fwprintf(fp, L"  {\"type\": \"%s\", \"name\": \"%s\", \"ip\": \"%s\", \"time\": \"%s\", \"date\": \"%s\"}",
                 type, name, ip, timeStr, dateStr);

        // 기존 항목 추가
        if (existingData && fileSize > 4)
        {
            // "[" 와 "]" 사이의 내용 찾기
            char *start = strchr(existingData, '[');
            if (start)
            {
                start++;
                // 앞쪽 공백/개행 건너뛰기
                while (*start && (*start == ' ' || *start == '\n' || *start == '\r' || *start == '\t'))
                    start++;

                char *end = strrchr(start, ']');
                if (end && end > start)
                {
                    *end = '\0';
                    // 뒤쪽 공백/개행 제거
                    while (end > start && (*(end - 1) == ' ' || *(end - 1) == '\n' || *(end - 1) == '\r' || *(end - 1) == '\t'))
                    {
                        end--;
                        *end = '\0';
                    }

                    if (strlen(start) > 0)
                    {
                        fwprintf(fp, L",\n");
                        // UTF-8 문자열을 wide string으로 변환
                        int wideLen = MultiByteToWideChar(CP_UTF8, 0, start, -1, NULL, 0);
                        if (wideLen > 0)
                        {
                            wchar_t *wideStr = (wchar_t *)malloc(wideLen * sizeof(wchar_t));
                            if (wideStr)
                            {
                                MultiByteToWideChar(CP_UTF8, 0, start, -1, wideStr, wideLen);
                                fwprintf(fp, L"%s", wideStr);
                                free(wideStr);
                            }
                        }
                    }
                }
            }
        }

        fwprintf(fp, L"\n]\n");
        fclose(fp);
        wprintf(L"[로그] 알림 기록 저장 완료: %s\n", logPath);
    }
    else
    {
        wprintf(L"[로그] 알림 기록 파일 열기 실패: %s\n", logPath);
    }

    if (existingData)
        free(existingData);

    LeaveCriticalSection(&g_logLock);
}