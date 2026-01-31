#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include "types.h"

// Custom notification window
#define WM_SHOW_NOTIFICATION (WM_USER + 100)
#define NOTIFICATION_DISPLAY_TIME 5000
#define NOTIFICATION_WIDTH 350
#define NOTIFICATION_HEIGHT 100

typedef struct
{
    HWND hwnd;
    wchar_t title[128];
    wchar_t message[256];
    DWORD type;
    DWORD startTime;
} NotificationWindow;

typedef struct
{
    wchar_t title[128];
    wchar_t message[256];
    DWORD type;
} NotificationRequest;

void LoadNotificationSettings(void);
void ShowBalloonNotification(const wchar_t *title, const wchar_t *message, DWORD infoFlags);
void CheckAndNotify(IPTarget *target);
void SaveNotificationLog(const wchar_t *type, const wchar_t *name, const wchar_t *ip, const wchar_t *timeStr);

// Custom notification functions
void InitNotificationSystem(void);
void ShowCustomNotification(const wchar_t *title, const wchar_t *message, DWORD type);
void ProcessShowNotification(NotificationRequest *req);
void CleanupNotificationSystem(void);

#endif