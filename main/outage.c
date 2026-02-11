/**
 * Outage Management Module
 *
 * v2.7 로그 로테이션:
 * - 최대 로그 파일 크기: 5MB
 * - 백업 파일 개수: 3개 (log.json → log.json.1 → log.json.2 → log.json.3)
 * - 30일 이상 된 백업 파일 자동 삭제
 * - 1시간마다 로테이션 체크
 */

#include "outage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shlwapi.h>
#include <sys/stat.h>

// ============================================
// v2.7 로그 로테이션 설정
// ============================================
#define MAX_LOG_SIZE_MB 5 // 최대 로그 파일 크기 (MB)
#define MAX_LOG_SIZE_BYTES (MAX_LOG_SIZE_MB * 1024 * 1024)
#define MAX_BACKUP_COUNT 3           // 백업 파일 개수
#define BACKUP_RETENTION_DAYS 30     // 백업 보관 기간 (일)
#define ROTATION_CHECK_INTERVAL 3600 // 로테이션 체크 간격 (초)

static time_t s_lastRotationCheck = 0;

// ============================================
// 기존 코드 (원본 유지)
// ============================================

static OutageConfig g_outageConfig = {0};
static CRITICAL_SECTION g_outageLock;

void InitOutageSystem(void)
{
    InitializeCriticalSection(&g_outageLock);
    g_outageConfig.defaultThreshold = 300;
    s_lastRotationCheck = time(NULL);
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

// ============================================
// v2.7 로그 로테이션 함수
// ============================================

/**
 * 파일 크기 확인 (바이트)
 */
static long long GetFileSizeBytes(const wchar_t *filePath)
{
    struct _stat64 st;
    if (_wstat64(filePath, &st) == 0)
    {
        return st.st_size;
    }
    return 0;
}

/**
 * 파일 수정 시간 확인
 */
static time_t GetFileModTime(const wchar_t *filePath)
{
    struct _stat64 st;
    if (_wstat64(filePath, &st) == 0)
    {
        return st.st_mtime;
    }
    return 0;
}

/**
 * 단일 로그 파일 로테이션
 * log.json → log.json.1 → log.json.2 → log.json.3 (삭제)
 */
static void RotateLogFile(const wchar_t *logPath)
{
    wchar_t backupPath[MAX_PATH];
    wchar_t oldBackupPath[MAX_PATH];

    // 가장 오래된 백업 삭제 (log.json.3)
    swprintf(backupPath, MAX_PATH, L"%s.%d", logPath, MAX_BACKUP_COUNT);
    DeleteFileW(backupPath);

    // 백업 파일들 이름 변경 (3 ← 2 ← 1)
    for (int i = MAX_BACKUP_COUNT - 1; i >= 1; i--)
    {
        swprintf(oldBackupPath, MAX_PATH, L"%s.%d", logPath, i);
        swprintf(backupPath, MAX_PATH, L"%s.%d", logPath, i + 1);
        MoveFileW(oldBackupPath, backupPath);
    }

    // 현재 로그 → .1 백업
    swprintf(backupPath, MAX_PATH, L"%s.1", logPath);
    MoveFileW(logPath, backupPath);

    wprintf(L"[로그 로테이션] %s → %s.1\n", logPath, logPath);
}

/**
 * 오래된 백업 파일 정리 (단일 로그 파일용 - 내부)
 */
static void CleanupOldBackupsForLog(const wchar_t *logPath)
{
    time_t now = time(NULL);
    time_t cutoffTime = now - (BACKUP_RETENTION_DAYS * 24 * 60 * 60);

    wchar_t backupPath[MAX_PATH];

    for (int i = 1; i <= MAX_BACKUP_COUNT; i++)
    {
        swprintf(backupPath, MAX_PATH, L"%s.%d", logPath, i);

        time_t modTime = GetFileModTime(backupPath);
        if (modTime > 0 && modTime < cutoffTime)
        {
            if (DeleteFileW(backupPath))
            {
                wprintf(L"[로그 정리] 오래된 백업 삭제: %s\n", backupPath);
            }
        }
    }
}

/**
 * 로그 파일 로테이션 필요 여부 확인 및 실행
 */
static void CheckAndRotateLog(const wchar_t *logPath)
{
    long long fileSize = GetFileSizeBytes(logPath);

    if (fileSize >= MAX_LOG_SIZE_BYTES)
    {
        RotateLogFile(logPath);
    }
}

/**
 * 모든 로그 파일 로테이션 체크 (1시간마다)
 */
void CheckAllLogsRotation(void)
{
    time_t now = time(NULL);

    // 1시간마다만 체크
    if (difftime(now, s_lastRotationCheck) < ROTATION_CHECK_INTERVAL)
    {
        return;
    }

    s_lastRotationCheck = now;

    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    wchar_t logPath[MAX_PATH];

    // outage_log.json 로테이션 체크
    swprintf(logPath, MAX_PATH, L"%s\\data\\outage_log.json", exeDir);
    CheckAndRotateLog(logPath);
    CleanupOldBackupsForLog(logPath);

    // notification_log.json 로테이션 체크
    swprintf(logPath, MAX_PATH, L"%s\\data\\notification_log.json", exeDir);
    CheckAndRotateLog(logPath);
    CleanupOldBackupsForLog(logPath);
}

// ============================================
// 기존 코드 (원본 유지) + 로테이션 호출 추가
// ============================================

void UpdateTargetOutageStatus(OutageTarget *target, BOOL pingSuccess)
{
    time_t now = time(NULL);

    // v2.7: 주기적 로테이션 체크
    CheckAllLogsRotation();

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
    PathRemoveFileSpecW(exeDir);

    wchar_t logPath[MAX_PATH];
    swprintf(logPath, MAX_PATH, L"%s\\data\\outage_log.json", exeDir);

    // v2.7: 기록 전 로테이션 체크
    CheckAndRotateLog(logPath);

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
    EnterCriticalSection(&g_outageLock);

    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    wchar_t logPath[MAX_PATH];
    swprintf(logPath, MAX_PATH, L"%s\\data\\outage_log.json", exeDir);

    // v2.7: 기록 전 로테이션 체크
    CheckAndRotateLog(logPath);

    FILE *fp = _wfopen(logPath, L"a, ccs=UTF-8");
    if (fp)
    {
        fwprintf(fp, L"{\"ip\":\"%s\",\"name\":\"%s\",\"end\":\"%lld\",\"status\":\"recovered\"}\n",
                 target->ip, target->name, (long long)endTime);
        fclose(fp);
    }

    LeaveCriticalSection(&g_outageLock);
}

/**
 * v2.7: 미응답 시스템 정리 (프로그램 종료 시)
 */
void CleanupOutageSystem(void)
{
    DeleteCriticalSection(&g_outageLock);
}

/**
 * v2.7: 오래된 백업 파일 정리 (30일 이상) - Public
 */
void CleanupOldBackups(void)
{
    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    wchar_t logPath[MAX_PATH];

    swprintf(logPath, MAX_PATH, L"%s\\data\\outage_log.json", exeDir);
    CleanupOldBackupsForLog(logPath);

    swprintf(logPath, MAX_PATH, L"%s\\data\\notification_log.json", exeDir);
    CleanupOldBackupsForLog(logPath);
}

/**
 * v2.7: 강제 로그 로테이션 (수동 호출)
 */
void ForceLogRotation(void)
{
    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(NULL, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    wchar_t logPath[MAX_PATH];

    swprintf(logPath, MAX_PATH, L"%s\\data\\outage_log.json", exeDir);
    RotateLogFile(logPath);

    swprintf(logPath, MAX_PATH, L"%s\\data\\notification_log.json", exeDir);
    RotateLogFile(logPath);

    wprintf(L"[강제 로테이션] 모든 로그 파일 로테이션 완료\n");
}