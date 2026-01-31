/**
 * Network Module Implementation
 * ICMP ping and monitoring
 */

#include "network.h"
#include "notification.h"
#include "../outage.h"
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>

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

    UpdateTargetOutageStatus(&target->outage, success);
}

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

void SendDataToWebView(void)
{
    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    wchar_t jsonPath[MAX_PATH];
    wchar_t tmpPath[MAX_PATH];
    swprintf(jsonPath, MAX_PATH, L"%s\\data\\ping_data.json", exeDir);
    swprintf(tmpPath, MAX_PATH, L"%s\\data\\ping_data.json.tmp", exeDir);

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

void StopMonitoring(void)
{
    if (!g_isRunning)
        return;

    g_isRunning = FALSE;
    wprintf(L"모니터링 중지\n");

    wcscpy(g_nid.szTip, L"Ping Monitor - 중지됨");
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}