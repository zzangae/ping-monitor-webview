/**
 * Notification Module
 * Balloon notifications and logging
 */

#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include "types.h"

// ============================================================================
// Function Declarations
// ============================================================================

/**
 * Load notification settings from config file
 */
void LoadNotificationSettings(void);

/**
 * Show balloon notification in system tray
 * @param title Notification title
 * @param message Notification message
 * @param infoFlags Icon flags (NIIF_INFO, NIIF_WARNING, NIIF_ERROR)
 */
void ShowBalloonNotification(const wchar_t *title, const wchar_t *message, DWORD infoFlags);

/**
 * Check target status and send notifications
 * @param target Target to check
 */
void CheckAndNotify(IPTarget *target);

/**
 * Save notification to log file
 * @param type Notification type ("timeout" or "recovery")
 * @param name Target name
 * @param ip Target IP
 * @param timeStr Time string
 */
void SaveNotificationLog(const wchar_t *type, const wchar_t *name, const wchar_t *ip, const wchar_t *timeStr);

#endif // NOTIFICATION_H