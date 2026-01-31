/**
 * Tray Icon Module
 * System tray icon and context menu management
 */

#ifndef TRAY_H
#define TRAY_H

#include "types.h"

// ============================================================================
// Function Declarations
// ============================================================================

/**
 * Initialize system tray icon
 * @param hwnd Window handle
 */
void InitTrayIcon(HWND hwnd);

/**
 * Remove system tray icon
 */
void RemoveTrayIcon(void);

/**
 * Show tray context menu
 * @param hwnd Window handle
 */
void ShowTrayMenu(HWND hwnd);

#endif // TRAY_H