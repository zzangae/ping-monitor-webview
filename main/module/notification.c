/**
 * Notification Module Implementation
 */

#include "notification.h"
#include <shellapi.h>
#include <shlwapi.h>
#include <stdio.h>
#include <time.h>

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