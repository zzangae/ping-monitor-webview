/**
 * Network Module Implementation
 * ICMP ping and monitoring
 *
 * v2.7 최적화:
 * - 정적 ICMP 핸들 재사용 (매번 생성/삭제 방지)
 * - 정적 버퍼 재사용 (malloc/free 반복 방지)
 * - JSON 버퍼 재사용
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

// ============================================
// v2.7 최적화: 정적 리소스 (365일 운영 안정성)
// ============================================
static HANDLE s_hIcmpFile = INVALID_HANDLE_VALUE;
static LPVOID s_replyBuffer = NULL;
static DWORD s_replyBufferSize = 0;
static wchar_t *s_jsonBuffer = NULL;
static size_t s_jsonBufferSize = 0;
static BOOL s_optimizedResourcesReady = FALSE;

/**
 * 최적화된 리소스 초기화 (최초 1회)
 */
static BOOL EnsureOptimizedResources(void)
{
    if (s_optimizedResourcesReady)
        return TRUE;

    // ICMP 핸들 생성 (재사용)
    if (s_hIcmpFile == INVALID_HANDLE_VALUE)
    {
        s_hIcmpFile = IcmpCreateFile();
        if (s_hIcmpFile == INVALID_HANDLE_VALUE)
        {
            return FALSE;
        }
    }

    // Reply 버퍼 할당 (재사용)
    if (s_replyBuffer == NULL)
    {
        s_replyBufferSize = sizeof(ICMP_ECHO_REPLY) + 32 + 8;
        s_replyBuffer = malloc(s_replyBufferSize);
        if (s_replyBuffer == NULL)
        {
            IcmpCloseHandle(s_hIcmpFile);
            s_hIcmpFile = INVALID_HANDLE_VALUE;
            return FALSE;
        }
    }

    // JSON 버퍼 할당 (재사용)
    if (s_jsonBuffer == NULL)
    {
        s_jsonBufferSize = 10 * 1024 * 1024; // 10MB
        s_jsonBuffer = (wchar_t *)malloc(s_jsonBufferSize);
        if (s_jsonBuffer == NULL)
        {
            free(s_replyBuffer);
            s_replyBuffer = NULL;
            IcmpCloseHandle(s_hIcmpFile);
            s_hIcmpFile = INVALID_HANDLE_VALUE;
            return FALSE;
        }
    }

    s_optimizedResourcesReady = TRUE;
    return TRUE;
}

/**
 * 최적화된 리소스 정리 (프로그램 종료 시)
 */
void CleanupNetworkModule(void)
{
    if (s_jsonBuffer)
    {
        free(s_jsonBuffer);
        s_jsonBuffer = NULL;
    }

    if (s_replyBuffer)
    {
        free(s_replyBuffer);
        s_replyBuffer = NULL;
    }

    if (s_hIcmpFile != INVALID_HANDLE_VALUE)
    {
        IcmpCloseHandle(s_hIcmpFile);
        s_hIcmpFile = INVALID_HANDLE_VALUE;
    }

    s_optimizedResourcesReady = FALSE;
}

/**
 * 네트워크 모듈 초기화
 */
BOOL InitNetworkModule(void)
{
    return EnsureOptimizedResources();
}

// ============================================
// 기존 코드 (원본 유지)
// ============================================

BOOL DoPing(const wchar_t *ip, DWORD *latency)
{
    // v2.7: 최적화된 경로 시도
    if (EnsureOptimizedResources())
    {
        char ipStr[64];
        WideCharToMultiByte(CP_UTF8, 0, ip, -1, ipStr, sizeof(ipStr), NULL, NULL);

        unsigned long ipAddr = inet_addr(ipStr);
        if (ipAddr == INADDR_NONE)
        {
            return FALSE;
        }

        char sendData[32] = "PingMonitorData";

        DWORD result = IcmpSendEcho(
            s_hIcmpFile,
            ipAddr,
            sendData,
            sizeof(sendData),
            NULL,
            s_replyBuffer,
            s_replyBufferSize,
            PING_TIMEOUT);

        if (result != 0)
        {
            PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)s_replyBuffer;
            if (pEchoReply->Status == IP_SUCCESS)
            {
                *latency = pEchoReply->RoundTripTime;
                return TRUE;
            }
        }

        return FALSE;
    }

    // Fallback: 기존 방식 (최적화 실패 시)
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

    // v2.7: 정적 버퍼 사용 (최적화)
    wchar_t *jsonData = NULL;
    BOOL useStaticBuffer = FALSE;

    if (EnsureOptimizedResources() && s_jsonBuffer)
    {
        jsonData = s_jsonBuffer;
        useStaticBuffer = TRUE;
    }
    else
    {
        // Fallback: 동적 할당
        size_t bufferSize = 10 * 1024 * 1024;
        jsonData = (wchar_t *)malloc(bufferSize);
        if (!jsonData)
            return;
    }

    BuildJsonData(jsonData, (useStaticBuffer ? s_jsonBufferSize : 10 * 1024 * 1024) / sizeof(wchar_t));

    FILE *tmpFile = _wfopen(tmpPath, L"w, ccs=UTF-8");
    if (tmpFile)
    {
        fwprintf(tmpFile, L"%s", jsonData);
        fclose(tmpFile);

        DeleteFileW(jsonPath);
        MoveFileW(tmpPath, jsonPath);
    }

    // 동적 할당한 경우만 해제
    if (!useStaticBuffer && jsonData)
    {
        free(jsonData);
    }
}

DWORD WINAPI MonitoringThread(LPVOID lpParam)
{
    // v2.7: 모니터링 시작 시 리소스 초기화
    EnsureOptimizedResources();

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