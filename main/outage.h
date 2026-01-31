#ifndef OUTAGE_H
#define OUTAGE_H

#include <windows.h>
#include <time.h>

#define MAX_IP_LEN 64
#define MAX_NAME_LEN 128
#define MAX_GROUP_LEN 64

typedef struct
{
    wchar_t ip[MAX_IP_LEN];
    wchar_t name[MAX_NAME_LEN];
    BOOL isDown;
    time_t downStartTime;
    int consecutiveFailures;
    int outageThreshold;
    BOOL outageConfirmed;
    time_t outageStartTime;
    wchar_t group[MAX_GROUP_LEN];
    int priority;
} OutageTarget;

typedef struct
{
    int defaultThreshold;
    int serverThreshold;
    int networkThreshold;
    int firewallThreshold;
} OutageConfig;

void InitOutageSystem(void);
void UpdateTargetOutageStatus(OutageTarget *target, BOOL pingSuccess);
void RecordOutageStart(OutageTarget *target, time_t startTime);
void RecordOutageEnd(OutageTarget *target, time_t endTime);
void LoadOutageConfig(const wchar_t *configPath);
void ParseIPGroup(const wchar_t *ip, const wchar_t *configPath, wchar_t *group, int *priority);

#endif