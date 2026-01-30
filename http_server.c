/**
 * HTTP Server for Ping Monitor
 * Simple embedded HTTP server implementation
 */

#include "http_server.h"
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

// ============================================================================
// Global Variables
// ============================================================================
static SOCKET g_serverSocket = INVALID_SOCKET;
static HANDLE g_serverThread = NULL;
static BOOL g_serverRunning = FALSE;
static int g_serverPort = HTTP_PORT_DEFAULT;
static WCHAR g_rootPath[HTTP_MAX_PATH] = {0};
static DWORD g_lastRequestTime = 0; // For browser close detection

// ============================================================================
// Internal Function Declarations
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
// Start HTTP Server
// ============================================================================
BOOL StartHttpServer(int port, const WCHAR *rootPath)
{
    if (g_serverRunning)
    {
        return TRUE; // Already running
    }

    g_serverPort = port;
    wcscpy(g_rootPath, rootPath);

    // Create socket
    g_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_serverSocket == INVALID_SOCKET)
    {
        return FALSE;
    }

    // Set SO_REUSEADDR
    int opt = 1;
    setsockopt(g_serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    // Bind
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost only
    addr.sin_port = htons((u_short)port);

    if (bind(g_serverSocket, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(g_serverSocket);
        g_serverSocket = INVALID_SOCKET;
        return FALSE;
    }

    // Listen
    if (listen(g_serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        closesocket(g_serverSocket);
        g_serverSocket = INVALID_SOCKET;
        return FALSE;
    }

    g_serverRunning = TRUE;

    // Start server thread
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
// Stop HTTP Server
// ============================================================================
void StopHttpServer(void)
{
    g_serverRunning = FALSE;

    if (g_serverSocket != INVALID_SOCKET)
    {
        // Close socket (release accept blocking)
        shutdown(g_serverSocket, SD_BOTH);
        closesocket(g_serverSocket);
        g_serverSocket = INVALID_SOCKET;
    }

    if (g_serverThread)
    {
        // Wait for thread to terminate (max 2 seconds)
        if (WaitForSingleObject(g_serverThread, 2000) == WAIT_TIMEOUT)
        {
            // Force terminate on timeout
            TerminateThread(g_serverThread, 0);
        }
        CloseHandle(g_serverThread);
        g_serverThread = NULL;
    }
}

// ============================================================================
// Check Status
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
// Get Last Request Time (for browser close detection)
// ============================================================================
DWORD GetLastRequestTime(void)
{
    if (g_lastRequestTime == 0)
    {
        return 0; // Never requested
    }

    return (GetTickCount() - g_lastRequestTime) / 1000; // Seconds since last request
}

// ============================================================================
// Reset Last Request Time
// ============================================================================
void ResetLastRequestTime(void)
{
    g_lastRequestTime = GetTickCount();
}

// ============================================================================
// Server Thread
// ============================================================================
static DWORD WINAPI HttpServerThread(LPVOID lpParam)
{
    (void)lpParam;

    while (g_serverRunning)
    {
        // Wait for client connection
        struct sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);

        SOCKET clientSocket = accept(g_serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);

        if (clientSocket == INVALID_SOCKET)
        {
            if (!g_serverRunning)
                break; // Server stopped
            continue;
        }

        // Handle client
        HandleClient(clientSocket);
        closesocket(clientSocket);
    }

    return 0;
}

// ============================================================================
// Handle Client Request
// ============================================================================
static void HandleClient(SOCKET clientSocket)
{
    // Record request time for browser close detection
    g_lastRequestTime = GetTickCount();

    char buffer[HTTP_BUFFER_SIZE];
    int received = recv(clientSocket, buffer, HTTP_BUFFER_SIZE - 1, 0);

    if (received <= 0)
    {
        return;
    }

    buffer[received] = '\0';

    // Parse GET request
    if (strncmp(buffer, "GET ", 4) != 0)
    {
        Send404(clientSocket);
        return;
    }

    // Extract URL
    char *urlStart = buffer + 4;
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

    // Remove query string
    char *query = strchr(url, '?');
    if (query)
        *query = '\0';

    // Default page
    if (strcmp(url, "/") == 0)
    {
        strcpy(url, "/graph.html");
    }

    // Build file path
    char filePath[HTTP_MAX_PATH * 2];
    char rootPathAnsi[HTTP_MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, g_rootPath, -1, rootPathAnsi, HTTP_MAX_PATH, NULL, NULL);

    snprintf(filePath, sizeof(filePath), "%s%s", rootPathAnsi, url);

    // Normalize path (prevent ..)
    if (strstr(filePath, ".."))
    {
        Send404(clientSocket);
        return;
    }

    // Convert to backslash
    for (char *p = filePath; *p; p++)
    {
        if (*p == '/')
            *p = '\\';
    }

    // Send file
    SendFile(clientSocket, filePath);
}

// ============================================================================
// Determine MIME Type
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
// Send HTTP Response
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
// Send File
// ============================================================================
static void SendFile(SOCKET client, const char *filePath)
{
    FILE *fp = fopen(filePath, "rb");
    if (!fp)
    {
        Send404(client);
        return;
    }

    // Check file size
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (fileSize <= 0 || fileSize > 10 * 1024 * 1024)
    { // Max 10MB
        fclose(fp);
        Send500(client);
        return;
    }

    // Read file
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

    // Determine MIME type and send
    const char *mimeType = GetMimeType(filePath);
    SendResponse(client, 200, "OK", mimeType, fileContent, (int)fileSize);

    free(fileContent);
}

// ============================================================================
// 404 Error
// ============================================================================
static void Send404(SOCKET client)
{
    const char *body =
        "<!DOCTYPE html><html><head><title>404 Not Found</title></head>"
        "<body><h1>404 Not Found</h1><p>The requested file was not found.</p></body></html>";
    SendResponse(client, 404, "Not Found", "text/html; charset=utf-8", body, (int)strlen(body));
}

// ============================================================================
// 500 Error
// ============================================================================
static void Send500(SOCKET client)
{
    const char *body =
        "<!DOCTYPE html><html><head><title>500 Internal Server Error</title></head>"
        "<body><h1>500 Internal Server Error</h1></body></html>";
    SendResponse(client, 500, "Internal Server Error", "text/html; charset=utf-8", body, (int)strlen(body));
}