/**
 * HTTP Server for Ping Monitor
 * 간단한 내장 HTTP 서버
 */

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <winsock2.h>
#include <windows.h>

// ============================================================================
// 상수
// ============================================================================
#define HTTP_PORT_DEFAULT 8080
#define HTTP_BUFFER_SIZE 8192
#define HTTP_MAX_PATH 512

// ============================================================================
// 함수 선언
// ============================================================================

/**
 * HTTP 서버 시작 (별도 스레드에서 실행)
 * @param port 포트 번호 (기본 8080)
 * @param rootPath 웹 루트 경로 (graph.html이 있는 폴더)
 * @return 성공 시 TRUE
 */
BOOL StartHttpServer(int port, const WCHAR *rootPath);

/**
 * HTTP 서버 중지
 */
void StopHttpServer(void);

/**
 * HTTP 서버 실행 중인지 확인
 * @return 실행 중이면 TRUE
 */
BOOL IsHttpServerRunning(void);

/**
 * HTTP 서버 포트 번호 가져오기
 * @return 포트 번호
 */
int GetHttpServerPort(void);

#endif // HTTP_SERVER_H