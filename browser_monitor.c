/**
 * Browser Process Monitor Implementation
 *
 * This module launches a browser using CreateProcess (instead of ShellExecuteW)
 * to obtain a process handle, enabling real-time monitoring of browser state.
 *
 * When the browser closes, the program can detect it within 1 second and
 * terminate gracefully while preserving all data files.
 */

#include "browser_monitor.h"
#include <stdio.h>
#include <wchar.h>

// ============================================================================
// Global Variables
// ============================================================================

HANDLE g_browserProcess = NULL; // Browser process handle
DWORD g_browserProcessId = 0;   // Browser process ID

// ============================================================================
// Internal Helper Functions
// ============================================================================

/**
 * Get default browser path from Windows registry
 *
 * Reads:
 *   1. HKCU\Software\Microsoft\Windows\Shell\Associations\
 *      UrlAssociations\http\UserChoice -> ProgId
 *   2. HKCR\{ProgId}\shell\open\command -> Browser executable path
 *
 * @param browserPath Output buffer for browser path
 * @param bufferSize Size of output buffer
 * @return TRUE if successful
 */
static BOOL GetDefaultBrowserPath(wchar_t *browserPath, DWORD bufferSize)
{
    HKEY hKey;
    wchar_t progId[256] = {0};
    DWORD progIdSize = sizeof(progId);

    // Step 1: Get ProgId
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\http\\UserChoice",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExW(hKey, L"ProgId", NULL, NULL,
                             (LPBYTE)progId, &progIdSize) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);

            // Step 2: Get browser executable path from ProgId
            wchar_t commandKey[512];
            swprintf(commandKey, 512, L"%s\\shell\\open\\command", progId);

            if (RegOpenKeyExW(HKEY_CLASSES_ROOT, commandKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                DWORD pathSize = bufferSize;
                BOOL success = (RegQueryValueExW(hKey, NULL, NULL, NULL,
                                                 (LPBYTE)browserPath, &pathSize) == ERROR_SUCCESS);
                RegCloseKey(hKey);

                return success;
            }
        }
        else
        {
            RegCloseKey(hKey);
        }
    }

    return FALSE;
}

/**
 * Clean browser path by removing quotes and parameters
 *
 * Input:  "C:\Program Files\Google\Chrome\Application\chrome.exe" "%1"
 * Output: C:\Program Files\Google\Chrome\Application\chrome.exe
 *
 * @param rawPath Raw path from registry
 * @param cleanPath Output buffer for cleaned path
 * @param bufferSize Size of output buffer
 * @return TRUE if successful
 */
static BOOL CleanBrowserPath(const wchar_t *rawPath, wchar_t *cleanPath,
                             DWORD bufferSize)
{
    if (!rawPath || !cleanPath || bufferSize == 0)
        return FALSE;

    // Look for quoted path
    const wchar_t *start = wcschr(rawPath, L'"');
    if (start)
    {
        start++; // Skip opening quote
        const wchar_t *end = wcschr(start, L'"');
        if (end && (end - start) < bufferSize)
        {
            wcsncpy(cleanPath, start, end - start);
            cleanPath[end - start] = L'\0';
            return TRUE;
        }
    }

    // No quotes - take until first space
    const wchar_t *space = wcschr(rawPath, L' ');
    if (space)
    {
        size_t len = space - rawPath;
        if (len < bufferSize)
        {
            wcsncpy(cleanPath, rawPath, len);
            cleanPath[len] = L'\0';
            return TRUE;
        }
    }

    // No spaces - copy entire string
    if (wcslen(rawPath) < bufferSize)
    {
        wcscpy(cleanPath, rawPath);
        return TRUE;
    }

    return FALSE;
}

// ============================================================================
// Public Functions
// ============================================================================

void OpenBrowser(const wchar_t *url)
{
    wprintf(L"\n========================================\n");
    wprintf(L"OpenBrowser() called\n");
    wprintf(L"URL: %s\n", url);
    wprintf(L"========================================\n");

    // Close previous browser process handle if exists
    if (g_browserProcess)
    {
        wprintf(L"Closing previous browser process handle...\n");
        CloseHandle(g_browserProcess);
        g_browserProcess = NULL;
        g_browserProcessId = 0;
    }

    // Get default browser path from registry
    wchar_t browserPath[MAX_PATH] = {0};

    if (!GetDefaultBrowserPath(browserPath, MAX_PATH))
    {
        // Fallback: Use ShellExecuteW
        wprintf(L"Could not read browser path from registry, using ShellExecuteW\n");
        ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);
        return;
    }

    // Clean up browser path (remove quotes and parameters)
    wchar_t cleanPath[MAX_PATH] = {0};

    if (!CleanBrowserPath(browserPath, cleanPath, MAX_PATH))
    {
        // Fallback: Use ShellExecuteW
        wprintf(L"Could not clean browser path, using ShellExecuteW\n");
        ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);
        return;
    }

    // Launch browser with CreateProcess to get process handle
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;

    // Build command line: "browser.exe" "url"
    wchar_t cmdLine[1024];
    swprintf(cmdLine, 1024, L"\"%s\" \"%s\"", cleanPath, url);

    if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        // Success - save process handle and ID
        g_browserProcess = pi.hProcess;
        g_browserProcessId = pi.dwProcessId;
        CloseHandle(pi.hThread); // Don't need thread handle

        wprintf(L"Browser launched successfully\n");
        wprintf(L"  Path: %s\n", cleanPath);
        wprintf(L"  PID: %d\n", g_browserProcessId);
        wprintf(L"  URL: %s\n", url);
    }
    else
    {
        // CreateProcess failed - use ShellExecuteW as fallback
        DWORD error = GetLastError();
        wprintf(L"CreateProcess failed (error: %d), using ShellExecuteW\n", error);
        ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);
    }
}

BOOL IsBrowserRunning(void)
{
    if (g_browserProcess == NULL)
        return FALSE;

    DWORD exitCode;
    if (!GetExitCodeProcess(g_browserProcess, &exitCode))
    {
        // Failed to get exit code - assume process ended
        return FALSE;
    }

    // STILL_ACTIVE (259) means process is still running
    return (exitCode == STILL_ACTIVE);
}

void CleanupBrowserMonitor(void)
{
    if (g_browserProcess)
    {
        wprintf(L"Cleaning up browser process handle (PID: %d)\n", g_browserProcessId);
        CloseHandle(g_browserProcess);
        g_browserProcess = NULL;
        g_browserProcessId = 0;
    }
}

DWORD GetBrowserProcessId(void)
{
    return g_browserProcessId;
}