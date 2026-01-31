/**
 * Common Types and Constants
 * Shared across all modules
 */

#ifndef TYPES_H
#define TYPES_H

#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "../outage.h"

// ============================================================================
// Constants
// ============================================================================

#define MAX_IP_COUNT 50
#define MAX_HISTORY 60
#define PING_TIMEOUT 2000

// ⭐ 수정: 새 폴더 구조 반영
#define JSON_FILE L"..\\..\\data\\ping_data.json"
#define CONFIG_FILE L"..\\..\\config\\ping_config.ini"
#define INT_CONFIG_FILE L"..\\..\\config\\int_config.ini"

// HTTP Server
#define HTTP_PORT 8080
#define HTTP_PORT_MIN 8080
#define HTTP_PORT_MAX 8099

// Browser monitoring
#define BROWSER_CHECK_INTERVAL 5000
#define BROWSER_GRACE_PERIOD 15000

// Tray Icon
#define ID_TRAY_ICON 1
#define WM_TRAYICON (WM_USER + 1)

// Menu IDs
#define ID_TRAY_START 2001
#define ID_TRAY_BROWSER 2002
#define ID_TRAY_NOTIFICATIONS 2003
#define ID_TRAY_RELOAD_CONFIG 2004
#define ID_TRAY_CHANGE_PORT 2005
#define ID_TRAY_EXIT 2006

// Timer IDs
#define ID_TIMER_BROWSER_CHECK 3001

// Notification defaults
#define DEFAULT_NOTIFICATION_ENABLED 1
#define DEFAULT_NOTIFICATION_COOLDOWN 60
#define DEFAULT_NOTIFY_ON_TIMEOUT 1
#define DEFAULT_NOTIFY_ON_RECOVERY 1
#define DEFAULT_CONSECUTIVE_FAILURES 3

// ============================================================================
// Structures
// ============================================================================

typedef struct
{
    wchar_t ip[64];
    wchar_t name[128];
    DWORD latency;
    int online;
    int total;
    int success;
    DWORD min;
    DWORD max;
    double avg;
    DWORD history[MAX_HISTORY];
    int historyIndex;

    time_t lastNotificationTime;
    int previousOnline;
    int consecutiveFailures;
    int consecutiveSuccesses;

    OutageTarget outage;
} IPTarget;

typedef struct
{
    DWORD pingInterval;
    int maxCount;
    BOOL loop;
} TimeSettings;

typedef struct
{
    BOOL enabled;
    int cooldown;
    BOOL notifyOnTimeout;
    BOOL notifyOnRecovery;
    int consecutiveFailuresThreshold;
} NotificationSettings;

// ============================================================================
// Global Variables (extern declarations)
// ============================================================================

extern IPTarget g_targets[MAX_IP_COUNT];
extern int g_targetCount;
extern TimeSettings g_timeSettings;
extern BOOL g_isRunning;
extern HWND g_hwnd;
extern HWND g_mainHwnd;
extern NOTIFYICONDATAW g_nid;
extern wchar_t g_exePath[MAX_PATH];
extern int g_currentPort;
extern DWORD g_browserStartTime;
extern NotificationSettings g_notifSettings;
extern CRITICAL_SECTION g_logLock;

#endif