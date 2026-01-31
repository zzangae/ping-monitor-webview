/**
 * Config Module Implementation
 */

#include "config.h"
#include "../config_api.h"
#include <shlwapi.h>
#include <stdio.h>
#include <string.h>

static BOOL g_anyFileExists = FALSE;

void LoadConfigFromFile(const wchar_t *configFile)
{
    wchar_t configPath[MAX_PATH];
    wchar_t exeDir[MAX_PATH];

    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    swprintf(configPath, MAX_PATH, L"%s\\config\\%s", exeDir, configFile);

    DWORD attr = GetFileAttributesW(configPath);
    if (attr == INVALID_FILE_ATTRIBUTES)
    {
        wprintf(L"설정 파일 없음: %s (무시)\n", configFile);
        return;
    }
    else
    {
        g_anyFileExists = TRUE;
    }

    FILE *file = _wfopen(configPath, L"r, ccs=UTF-8");
    if (!file)
    {
        file = _wfopen(configPath, L"r");
    }

    if (!file)
    {
        wprintf(L"설정 파일을 열 수 없습니다: %s\n", configPath);
        return;
    }

    wprintf(L"설정 파일 읽는 중: %s\n", configFile);

    wchar_t line[512];
    BOOL inSettingsSection = FALSE;
    int targetsParsed = 0;

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

        if (wcsstr(line, L"[Targets]") || wcsstr(line, L"[TARGETS]"))
        {
            inSettingsSection = FALSE;
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

            if (wcslen(ip) == 0)
                continue;

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

            wcscpy(g_targets[g_targetCount].outage.ip, ip);
            wcscpy(g_targets[g_targetCount].outage.name, name);
            g_targets[g_targetCount].outage.isDown = FALSE;
            g_targets[g_targetCount].outage.downStartTime = 0;
            g_targets[g_targetCount].outage.consecutiveFailures = 0;
            g_targets[g_targetCount].outage.outageThreshold = 300;
            g_targets[g_targetCount].outage.outageConfirmed = FALSE;
            g_targets[g_targetCount].outage.outageStartTime = 0;

            ParseIPGroup(ip, configPath,
                         g_targets[g_targetCount].outage.group,
                         &g_targets[g_targetCount].outage.priority);

            memset(g_targets[g_targetCount].history, 0, sizeof(g_targets[g_targetCount].history));

            g_targetCount++;
            targetsParsed++;
        }
    }

    fclose(file);
    wprintf(L"  %s에서 %d개 타겟 로드 완료\n", configFile, targetsParsed);
}

void LoadConfig(void)
{
    g_targetCount = 0;
    g_anyFileExists = FALSE;

    wprintf(L"==========================================\n");
    wprintf(L"설정 파일 로딩 시작\n");
    wprintf(L"==========================================\n");

    LoadConfigFromFile(L"ping_config.ini");

    wprintf(L"------------------------------------------\n");

    LoadConfigFromFile(L"int_config.ini");

    wprintf(L"==========================================\n");
    wprintf(L"설정 로드 완료: 총 %d개 타겟\n", g_targetCount);

    if (!g_anyFileExists)
    {
        MessageBoxW(NULL,
                    L"설정 파일이 없습니다.\n\n"
                    L"다음 파일 중 최소 1개가 필요합니다:\n"
                    L"  - config\\ping_config.ini\n"
                    L"  - config\\int_config.ini",
                    L"설정 파일 오류",
                    MB_OK | MB_ICONERROR);
    }
    else if (g_targetCount == 0)
    {
        MessageBoxW(NULL,
                    L"설정 파일은 존재하지만 IP가 없습니다.\n\n"
                    L"config\\ping_config.ini 또는 config\\int_config.ini 파일에\n"
                    L"[Targets] 섹션 아래에 다음 형식으로 IP를 추가하세요:\n\n"
                    L"[Targets]\n"
                    L"8.8.8.8,Google DNS\n"
                    L"1.1.1.1,Cloudflare DNS",
                    L"설정 오류",
                    MB_OK | MB_ICONWARNING);
    }
    else
    {
        wprintf(L"알림 설정: %s (쿨다운: %d초, 연속실패: %d회)\n",
                g_notifSettings.enabled ? L"활성화" : L"비활성화",
                g_notifSettings.cooldown,
                g_notifSettings.consecutiveFailuresThreshold);
    }

    wprintf(L"==========================================\n");
}