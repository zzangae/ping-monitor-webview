/**
 * Outage Management Header (v2.7 - Log Rotation)
 */

#ifndef OUTAGE_H
#define OUTAGE_H

#include <windows.h>
#include <time.h>

// ============================================================================
// 로그 로테이션 설정 상수
// ============================================================================

#define MAX_LOG_SIZE_MB 5            // 최대 로그 파일 크기 (MB)
#define MAX_BACKUP_COUNT 3           // 백업 파일 최대 개수
#define ROTATION_CHECK_INTERVAL 3600 // 로테이션 체크 간격 (초)

// ============================================================================
// 구조체
// ============================================================================

typedef struct
{
    wchar_t ip[64];
    wchar_t name[128];
    wchar_t group[64];
    int priority;

    BOOL isDown;
    time_t downStartTime;
    int consecutiveFailures;

    BOOL outageConfirmed;
    time_t outageStartTime;
    int outageThreshold;
} OutageTarget;

typedef struct
{
    int defaultThreshold;
} OutageConfig;

// ============================================================================
// 초기화 및 정리
// ============================================================================

/**
 * 장애 관리 시스템 초기화
 */
void InitOutageSystem(void);

/**
 * 장애 관리 시스템 정리
 */
void CleanupOutageSystem(void);

/**
 * 장애 설정 로드
 */
void LoadOutageConfig(const wchar_t *configPath);

// ============================================================================
// 장애 추적
// ============================================================================

/**
 * IP 그룹 정보 파싱
 */
void ParseIPGroup(const wchar_t *ip, const wchar_t *configPath, wchar_t *group, int *priority);

/**
 * 대상 장애 상태 업데이트
 */
void UpdateTargetOutageStatus(OutageTarget *target, BOOL pingSuccess);

/**
 * 장애 시작 기록
 */
void RecordOutageStart(OutageTarget *target, time_t startTime);

/**
 * 장애 종료 기록
 */
void RecordOutageEnd(OutageTarget *target, time_t endTime);

// ============================================================================
// 로그 로테이션 (v2.7 신규)
// ============================================================================

/**
 * 모든 로그 파일 로테이션 체크 (1시간마다 자동 호출)
 */
void CheckAllLogsRotation(void);

/**
 * 강제 로그 로테이션 (수동 호출)
 */
void ForceLogRotation(void);

/**
 * 30일 이상 된 백업 파일 정리
 */
void CleanupOldBackups(void);

#endif // OUTAGE_H