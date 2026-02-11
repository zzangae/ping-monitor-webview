/**
 * Network Module Header (v2.7 Optimized)
 */

#ifndef NETWORK_H
#define NETWORK_H

#include "types.h"
#include <windows.h>

// ============================================================================
// 초기화 및 정리 (v2.7 신규)
// ============================================================================

/**
 * 네트워크 모듈 초기화
 * - ICMP 핸들 생성
 * - 버퍼 할당
 * @return 성공 시 TRUE
 */
BOOL InitNetworkModule(void);

/**
 * 네트워크 모듈 정리
 * - 모든 핸들 해제
 * - 메모리 해제
 */
void CleanupNetworkModule(void);

// ============================================================================
// Ping 함수
// ============================================================================

/**
 * ICMP Ping 실행
 * @param ip IP 주소 (wide string)
 * @param latency 응답 시간 (출력)
 * @return 성공 시 TRUE
 */
BOOL DoPing(const wchar_t *ip, DWORD *latency);

/**
 * 타겟 상태 업데이트
 * @param target 대상 IPTarget
 * @param success Ping 성공 여부
 * @param latency 응답 시간
 */
void UpdateTarget(IPTarget *target, BOOL success, DWORD latency);

// ============================================================================
// 데이터 전송
// ============================================================================

/**
 * JSON 데이터 생성
 * @param buffer 출력 버퍼
 * @param bufferSize 버퍼 크기 (문자 단위)
 */
void BuildJsonData(wchar_t *buffer, size_t bufferSize);

/**
 * WebView로 데이터 전송 (JSON 파일 갱신)
 */
void SendDataToWebView(void);

// ============================================================================
// 모니터링 제어
// ============================================================================

/**
 * 모니터링 스레드 함수
 */
DWORD WINAPI MonitoringThread(LPVOID lpParam);

/**
 * 모니터링 시작
 */
void StartMonitoring(void);

/**
 * 모니터링 중지
 */
void StopMonitoring(void);

#endif // NETWORK_H