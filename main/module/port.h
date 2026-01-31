/**
 * Port Management Module
 * Port availability check and server port change
 */

#ifndef PORT_H
#define PORT_H

#include "types.h"

// ============================================================================
// Function Declarations
// ============================================================================

/**
 * Check if port is available
 * @param port Port number to check
 * @return TRUE if available
 */
BOOL IsPortAvailable(int port);

/**
 * Find available port in range
 * @param startPort Start port
 * @param endPort End port
 * @return Available port number (0 if not found)
 */
int FindAvailablePort(int startPort, int endPort);

/**
 * Port change dialog procedure
 * @param hWnd Window handle
 * @param message Message
 * @param wParam WPARAM
 * @param lParam LPARAM
 * @return Dialog result
 */
LRESULT CALLBACK PortDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/**
 * Show port change dialog
 * @param hwnd Parent window handle
 */
void ChangeServerPort(HWND hwnd);

/**
 * Restart HTTP server with new port
 * @param newPort New port number
 * @return TRUE if successful
 */
BOOL RestartServer(int newPort);

#endif // PORT_H