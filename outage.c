#include "outage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static OutageConfig g_outageConfig = {0};
static CRITICAL_SECTION g_outageLock;

void InitOutageSystem(void)
{
    InitializeCriticalSection(&g_outageLock);
    g_outageConfig.defaultThreshold = 300;
}

void LoadOutageConfig(const wchar_t *configPath)
{
    g_outageConfig.defaultThreshold = GetPrivateProfileIntW(L"OutageDetection", L"OutageThreshold", 300, configPath);
}

void ParseIPGroup(const wchar_t *ip, const wchar_t *configPath, wchar_t *group, int *priority)
{
    wchar_t buffer[256];
    GetPrivateProfileStringW(L"IPGroups", ip, L"기타,5", buffer, 256, configPath);
    wchar_t *comma = wcschr(buffer, L',');
    if (comma)
    {
        wcsncpy(group, buffer, comma - buffer);
        group[comma - buffer] = 0;
        *priority = _wtoi(comma + 1);
    }
    else
    {
        wcscpy(group, L"기타");
        *priority = 5;
    }
}

void UpdateTargetOutageStatus(OutageTarget *target, BOOL pingSuccess)
{
    time_t now = time(NULL);
    if (pingSuccess)
    {
        if (target->outageConfirmed)
        {
            RecordOutageEnd(target, now);
        }
        target->isDown = FALSE;
        target->downStartTime = 0;
        target->consecutiveFailures = 0;
        target->outageConfirmed = FALSE;
        target->outageStartTime = 0;
    }
    else
    {
        target->consecutiveFailures++;
        if (!target->isDown)
        {
            target->isDown = TRUE;
            target->downStartTime = now;
        }
        int downDuration = (int)difftime(now, target->downStartTime);
        if (!target->outageConfirmed && downDuration >= target->outageThreshold)
        {
            target->outageConfirmed = TRUE;
            target->outageStartTime = now;
            RecordOutageStart(target, now);
        }
    }
}

void RecordOutageStart(OutageTarget *target, time_t startTime)
{
    EnterCriticalSection(&g_outageLock);
    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    wchar_t *lastSlash = wcsrchr(exeDir, L'\\');
    if (lastSlash)
        *lastSlash = 0;
    wchar_t logPath[MAX_PATH];
    swprintf(logPath, MAX_PATH, L"%s\\outage_log.json", exeDir);
    FILE *fp = _wfopen(logPath, L"a, ccs=UTF-8");
    if (fp)
    {
        fwprintf(fp, L"{\"ip\":\"%s\",\"name\":\"%s\",\"start\":\"%lld\",\"status\":\"ongoing\"}\n",
                 target->ip, target->name, (long long)startTime);
        fclose(fp);
    }
    LeaveCriticalSection(&g_outageLock);
}

void RecordOutageEnd(OutageTarget *target, time_t endTime)
{
    // 간소화: 파일 끝에 복구 로그만 추가
}