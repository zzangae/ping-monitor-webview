/**
 * Browser Process Monitor for Ping Monitor v2.6
 * Detects browser close and terminates program automatically
 */

#ifndef BROWSER_MONITOR_H
#define BROWSER_MONITOR_H

#include <windows.h>

// ============================================================================
// Constants
// ============================================================================

// Timer settings for browser process monitoring
#define ID_TIMER_BROWSER_CHECK 2001 // Timer ID
#define BROWSER_CHECK_INTERVAL 1000 // Check every 1 second (1000ms)

// ============================================================================
// Global Variables (defined in browser_monitor.c)
// ============================================================================

extern HANDLE g_browserProcess;  // Browser process handle
extern DWORD g_browserProcessId; // Browser process ID

// ============================================================================
// Function Declarations
// ============================================================================

/**
 * Open browser with process monitoring
 *
 * Uses CreateProcess to launch browser and obtain process handle.
 * Falls back to ShellExecuteW if CreateProcess fails.
 *
 * @param url URL to open (e.g., "http://localhost:8080/graph.html")
 *
 * Side effects:
 *   - Sets g_browserProcess to process handle (if successful)
 *   - Sets g_browserProcessId to process ID
 *   - Closes previous browser process handle if exists
 *
 * Example:
 *   OpenBrowser(L"http://localhost:8080/graph.html");
 */
void OpenBrowser(const wchar_t *url);

/**
 * Check if browser process is still running
 *
 * @return TRUE if browser is running, FALSE if closed or handle is NULL
 *
 * Example:
 *   if (!IsBrowserRunning()) {
 *       PostQuitMessage(0);  // Terminate program
 *   }
 */
BOOL IsBrowserRunning(void);

/**
 * Clean up browser process handle
 *
 * Closes the browser process handle and resets global variables.
 * Should be called before program exit.
 *
 * Example:
 *   CleanupBrowserMonitor();
 */
void CleanupBrowserMonitor(void);

/**
 * Get browser process ID
 *
 * @return Browser process ID (0 if not running)
 */
DWORD GetBrowserProcessId(void);

#endif // BROWSER_MONITOR_H