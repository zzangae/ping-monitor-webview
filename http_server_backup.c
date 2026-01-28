/**
 * HTTP Server for Ping Monitor
 * 간단한 내장 HTTP 서버 구현
 */

#include "http_server.h"
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

// 외부 변수 선언 (메인 윈도우 핸들)
extern HWND g_mainHwnd;

// ============================================================================
// 전역 변수
// ============================================================================
static SOCKET g_serverSocket = INVALID_SOCKET;
static HANDLE g_serverThread = NULL;
static BOOL g_serverRunning = FALSE;
static int g_serverPort = HTTP_PORT_DEFAULT;
static WCHAR g_rootPath[HTTP_MAX_PATH] = {0};

// ============================================================================
// 내부 함수 선언
// ============================================================================
static DWORD WINAPI HttpServerThread(LPVOID lpParam);
static void HandleClient(SOCKET clientSocket);
static const char *GetMimeType(const char *path);
static void SendResponse(SOCKET client, int statusCode, const char *statusText,
                         const char *contentType, const char *body, int bodyLen);
static void SendFile(SOCKET client, const char *filePath);
static void Send404(SOCKET client);
static void Send500(SOCKET client);

// ============================================================================
// HTTP 서버 시작
// ============================================================================
BOOL StartHttpServer(int port, const WCHAR *rootPath)
{
    if (g_serverRunning)
    {
        return TRUE; // 이미 실행 중
    }

    g_serverPort = port;
    wcscpy(g_rootPath, rootPath);

    // 소켓 생성
    g_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_serverSocket == INVALID_SOCKET)
    {
        return FALSE;
    }

    // SO_REUSEADDR 설정
    int opt = 1;
    setsockopt(g_serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    // 바인드
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost만
    addr.sin_port = htons((u_short)port);

    if (bind(g_serverSocket, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(g_serverSocket);
        g_serverSocket = INVALID_SOCKET;
        return FALSE;
    }

    // 리슨
    if (listen(g_serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        closesocket(g_serverSocket);
        g_serverSocket = INVALID_SOCKET;
        return FALSE;
    }

    g_serverRunning = TRUE;

    // 서버 스레드 시작
    g_serverThread = CreateThread(NULL, 0, HttpServerThread, NULL, 0, NULL);
    if (!g_serverThread)
    {
        closesocket(g_serverSocket);
        g_serverSocket = INVALID_SOCKET;
        g_serverRunning = FALSE;
        return FALSE;
    }

    return TRUE;
}

// ============================================================================
// HTTP 서버 중지
// ============================================================================
void StopHttpServer(void)
{
    g_serverRunning = FALSE;

    if (g_serverSocket != INVALID_SOCKET)
    {
        // 소켓 종료 (accept 블로킹 해제)
        shutdown(g_serverSocket, SD_BOTH);
        closesocket(g_serverSocket);
        g_serverSocket = INVALID_SOCKET;
    }

    if (g_serverThread)
    {
        // 스레드 종료 대기 (최대 2초)
        if (WaitForSingleObject(g_serverThread, 2000) == WAIT_TIMEOUT)
        {
            // 타임아웃 시 강제 종료
            TerminateThread(g_serverThread, 0);
        }
        CloseHandle(g_serverThread);
        g_serverThread = NULL;
    }
}

// ============================================================================
// 상태 확인
// ============================================================================
BOOL IsHttpServerRunning(void)
{
    return g_serverRunning;
}

int GetHttpServerPort(void)
{
    return g_serverPort;
}

// ============================================================================
// 서버 스레드
// ============================================================================
static DWORD WINAPI HttpServerThread(LPVOID lpParam)
{
    (void)lpParam;

    while (g_serverRunning)
    {
        // 클라이언트 연결 대기
        struct sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);

        SOCKET clientSocket = accept(g_serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);

        if (clientSocket == INVALID_SOCKET)
        {
            if (!g_serverRunning)
                break; // 서버 종료
            continue;
        }

        // 클라이언트 처리
        HandleClient(clientSocket);
        closesocket(clientSocket);
    }

    return 0;
}

// ============================================================================
// 클라이언트 요청 처리
// ============================================================================
static void HandleClient(SOCKET clientSocket)
{
    char buffer[HTTP_BUFFER_SIZE];
    int received = recv(clientSocket, buffer, HTTP_BUFFER_SIZE - 1, 0);

    if (received <= 0)
    {
        return;
    }

    buffer[received] = '\0';

    // GET 또는 POST 요청 파싱
    BOOL isPost = FALSE;
    if (strncmp(buffer, "POST ", 5) == 0)
    {
        isPost = TRUE;
    }
    else if (strncmp(buffer, "GET ", 4) != 0)
    {
        Send404(clientSocket);
        return;
    }

    // URL 추출
    char *urlStart = buffer + (isPost ? 5 : 4);
    char *urlEnd = strchr(urlStart, ' ');
    if (!urlEnd)
    {
        Send404(clientSocket);
        return;
    }

    *urlEnd = '\0';
    char url[HTTP_MAX_PATH];
    strncpy(url, urlStart, HTTP_MAX_PATH - 1);
    url[HTTP_MAX_PATH - 1] = '\0';

    // 쿼리 스트링 제거
    char *query = strchr(url, '?');
    if (query)
        *query = '\0';

    // /shutdown 엔드포인트 처리
    if (strcmp(url, "/shutdown") == 0)
    {
        const char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 2\r\n"
            "Connection: close\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "\r\n"
            "OK";

        send(clientSocket, response, (int)strlen(response), 0);

        // 메인 윈도우에 종료 메시지 전송
        if (g_mainHwnd)
        {
            PostMessage(g_mainHwnd, WM_CLOSE, 0, 0);
        }

        return;
    }

    // 기본 페이지
    if (strcmp(url, "/") == 0)
    {
        strcpy(url, "/graph.html");
    }

    // 파일 경로 생성
    char filePath[HTTP_MAX_PATH * 2];
    char rootPathAnsi[HTTP_MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, g_rootPath, -1, rootPathAnsi, HTTP_MAX_PATH, NULL, NULL);

    snprintf(filePath, sizeof(filePath), "%s%s", rootPathAnsi, url);

    // 경로 정규화 (.. 방지)
    if (strstr(filePath, ".."))
    {
        Send404(clientSocket);
        return;
    }

    // 백슬래시로 변환
    for (char *p = filePath; *p; p++)
    {
        if (*p == '/')
            *p = '\\';
    }

    // 파일 전송
    SendFile(clientSocket, filePath);
}

// ============================================================================
// MIME 타입 결정
// ============================================================================
static const char *GetMimeType(const char *path)
{
    const char *ext = strrchr(path, '.');
    if (!ext)
        return "application/octet-stream";

    if (_stricmp(ext, ".html") == 0 || _stricmp(ext, ".htm") == 0)
    {
        return "text/html; charset=utf-8";
    }
    else if (_stricmp(ext, ".css") == 0)
    {
        return "text/css; charset=utf-8";
    }
    else if (_stricmp(ext, ".js") == 0)
    {
        return "application/javascript; charset=utf-8";
    }
    else if (_stricmp(ext, ".json") == 0)
    {
        return "application/json; charset=utf-8";
    }
    else if (_stricmp(ext, ".png") == 0)
    {
        return "image/png";
    }
    else if (_stricmp(ext, ".jpg") == 0 || _stricmp(ext, ".jpeg") == 0)
    {
        return "image/jpeg";
    }
    else if (_stricmp(ext, ".gif") == 0)
    {
        return "image/gif";
    }
    else if (_stricmp(ext, ".svg") == 0)
    {
        return "image/svg+xml";
    }
    else if (_stricmp(ext, ".ico") == 0)
    {
        return "image/x-icon";
    }
    else if (_stricmp(ext, ".txt") == 0)
    {
        return "text/plain; charset=utf-8";
    }

    return "application/octet-stream";
}

// ============================================================================
// HTTP 응답 전송
// ============================================================================
static void SendResponse(SOCKET client, int statusCode, const char *statusText,
                         const char *contentType, const char *body, int bodyLen)
{
    char header[1024];
    int headerLen = snprintf(header, sizeof(header),
                             "HTTP/1.1 %d %s\r\n"
                             "Content-Type: %s\r\n"
                             "Content-Length: %d\r\n"
                             "Connection: close\r\n"
                             "Access-Control-Allow-Origin: *\r\n"
                             "Cache-Control: no-cache\r\n"
                             "\r\n",
                             statusCode, statusText, contentType, bodyLen);

    send(client, header, headerLen, 0);
    if (body && bodyLen > 0)
    {
        send(client, body, bodyLen, 0);
    }
}

// ============================================================================
// 파일 전송
// ============================================================================
static void SendFile(SOCKET client, const char *filePath)
{
    FILE *fp = fopen(filePath, "rb");
    if (!fp)
    {
        Send404(client);
        return;
    }

    // 파일 크기 확인
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (fileSize <= 0 || fileSize > 10 * 1024 * 1024)
    { // 최대 10MB
        fclose(fp);
        Send500(client);
        return;
    }

    // 파일 읽기
    char *fileContent = (char *)malloc(fileSize);
    if (!fileContent)
    {
        fclose(fp);
        Send500(client);
        return;
    }

    size_t bytesRead = fread(fileContent, 1, fileSize, fp);
    fclose(fp);

    if (bytesRead != (size_t)fileSize)
    {
        free(fileContent);
        Send500(client);
        return;
    }

    // MIME 타입 결정 및 전송
    const char *mimeType = GetMimeType(filePath);
    SendResponse(client, 200, "OK", mimeType, fileContent, (int)fileSize);

    free(fileContent);
}

// ============================================================================
// 404 에러
// ============================================================================
static void Send404(SOCKET client)
{
    const char *body =
        "<!DOCTYPE html><html><head><title>404 Not Found</title></head>"
        "<body><h1>404 Not Found</h1><p>The requested file was not found.</p></body></html>";
    SendResponse(client, 404, "Not Found", "text/html; charset=utf-8", body, (int)strlen(body));
}

// ============================================================================
// 500 에러
// ============================================================================
static void Send500(SOCKET client)
{
    const char *body =
        "<!DOCTYPE html><html><head><title>500 Internal Server Error</title></head>"
        "<body><h1>500 Internal Server Error</h1></body></html>";
    SendResponse(client, 500, "Internal Server Error", "text/html; charset=utf-8", body, (int)strlen(body));
}