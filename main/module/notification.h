/**
 * Notification Module Header (v2.7)
 * 상태 전환 기반 알림 시스템
 */

#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <winsock2.h> // windows.h 전에 포함 필수
#include <windows.h>
#include "types.h" // IPTarget 타입 정의

// ============================================================================
// Custom Message
// ============================================================================

#define WM_SHOW_NOTIFICATION (WM_USER + 100)

// ============================================================================
// Structures
// ============================================================================

typedef struct NotificationRequest
{
    wchar_t title[256];
    wchar_t message[512];
    BOOL isTimeout;
} NotificationRequest;

// ============================================================================
// Function Declarations
// ============================================================================

/**
 * 알림 시스템 초기화
 */
void InitNotificationSystem(void);

/**
 * 알림 시스템 정리 (모든 창 닫기, 리소스 해제)
 */
void CleanupNotificationSystem(void);

/**
 * 상태 전환 확인 및 알림 처리
 * - previousOnline → online 변화 감지
 * - 새로운 오프라인 전환: 팝업 + 기록
 * - 오프라인 지속: 기록만 (팝업 X)
 * - 복구: 팝업 + 기록
 *
 * @param target 대상 IP 정보
 */
void CheckAndNotify(IPTarget *target);

/**
 * 커스텀 알림창 표시 (PostMessage 방식)
 *
 * @param title 제목
 * @param name IP 이름
 * @param ip IP 주소
 * @param isTimeout TRUE=타임아웃(빨간색), FALSE=복구(녹색)
 */
void ShowCustomNotification(const wchar_t *title, const wchar_t *name, const wchar_t *ip, BOOL isTimeout);

/**
 * 메인 스레드에서 알림창 실제 생성
 * WM_SHOW_NOTIFICATION 메시지 처리용
 *
 * @param req 알림 요청 구조체 (free 호출 필요)
 */
void ProcessShowNotification(NotificationRequest *req);

#endif // NOTIFICATION_H