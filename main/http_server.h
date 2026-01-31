/**
 * HTTP Server for Ping Monitor
 * Simple embedded HTTP server
 */

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <winsock2.h>
#include <windows.h>

// ============================================================================
// Constants
// ============================================================================
#define HTTP_PORT_DEFAULT 8080
#define HTTP_BUFFER_SIZE 8192
#define HTTP_MAX_PATH 512

// ============================================================================
// Function Declarations
// ============================================================================

/**
 * Start HTTP server (runs in separate thread)
 * @param port Port number (default 8080)
 * @param rootPath Web root path (folder containing graph.html)
 * @return TRUE on success
 */
BOOL StartHttpServer(int port, const WCHAR *rootPath);

/**
 * Stop HTTP server
 */
void StopHttpServer(void);

/**
 * Check if HTTP server is running
 * @return TRUE if running
 */
BOOL IsHttpServerRunning(void);

/**
 * Get HTTP server port number
 * @return Port number
 */
int GetHttpServerPort(void);

/**
 * Get last HTTP request time (for browser close detection)
 * @return Time in seconds since last request (0 if never requested)
 */
DWORD GetLastRequestTime(void);

/**
 * Reset last request time to current time
 */
void ResetLastRequestTime(void);

#endif // HTTP_SERVER_H