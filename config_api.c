/**
 * Configuration API Implementation
 */

#include "config_api.h"
#include <stdio.h>
#include <string.h>
#include <wchar.h>

// ============================================================================
// Load Configuration
// ============================================================================
BOOL LoadOutageConfiguration(const wchar_t *configPath, OutageConfig *config)
{
    if (!config)
        return FALSE;

    // Initialize
    config->defaultThreshold = 300; // 5 minutes default
    config->groupCount = 0;

    // Read default threshold
    config->defaultThreshold = GetPrivateProfileIntW(
        L"OutageDetection", L"OutageThreshold", 300, configPath);

    // Read group thresholds
    wchar_t groupNames[][MAX_GROUP_NAME] = {
        L"서버", L"네트워크", L"방화벽", L"데이터베이스",
        L"웹서버", L"애플리케이션", L"스토리지", L"기타"};

    wchar_t keyNames[][64] = {
        L"ServerOutageThreshold",
        L"NetworkOutageThreshold",
        L"FirewallOutageThreshold",
        L"DatabaseOutageThreshold",
        L"WebServerOutageThreshold",
        L"ApplicationOutageThreshold",
        L"StorageOutageThreshold",
        L"OtherOutageThreshold"};

    for (int i = 0; i < 8; i++)
    {
        int threshold = GetPrivateProfileIntW(
            L"OutageDetection", keyNames[i], 0, configPath);

        if (threshold > 0 && config->groupCount < MAX_GROUPS)
        {
            wcscpy(config->groups[config->groupCount].groupName, groupNames[i]);
            config->groups[config->groupCount].thresholdSeconds = threshold;
            config->groupCount++;
        }
    }

    return TRUE;
}

// ============================================================================
// Save Configuration
// ============================================================================
BOOL SaveOutageConfiguration(const wchar_t *configPath, const OutageConfig *config)
{
    if (!config)
        return FALSE;

    // Save default threshold
    wchar_t buffer[32];
    swprintf(buffer, 32, L"%d", config->defaultThreshold);
    WritePrivateProfileStringW(L"OutageDetection", L"OutageThreshold", buffer, configPath);

    // Save group thresholds
    wchar_t keyMap[][2][64] = {
        {L"서버", L"ServerOutageThreshold"},
        {L"네트워크", L"NetworkOutageThreshold"},
        {L"방화벽", L"FirewallOutageThreshold"},
        {L"데이터베이스", L"DatabaseOutageThreshold"},
        {L"웹서버", L"WebServerOutageThreshold"},
        {L"애플리케이션", L"ApplicationOutageThreshold"},
        {L"스토리지", L"StorageOutageThreshold"},
        {L"기타", L"OtherOutageThreshold"}};

    for (int i = 0; i < config->groupCount; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (wcscmp(config->groups[i].groupName, keyMap[j][0]) == 0)
            {
                swprintf(buffer, 32, L"%d", config->groups[i].thresholdSeconds);
                WritePrivateProfileStringW(
                    L"OutageDetection", keyMap[j][1], buffer, configPath);
                break;
            }
        }
    }

    return TRUE;
}

// ============================================================================
// Get IP Group Configuration
// ============================================================================
BOOL GetIPGroupConfig(const wchar_t *configPath, const wchar_t *ip, IPGroupConfig *config)
{
    if (!config || !ip)
        return FALSE;

    wchar_t value[256];
    GetPrivateProfileStringW(L"IPGroups", ip, L"", value, 256, configPath);

    if (wcslen(value) == 0)
    {
        // Default values
        wcscpy(config->ip, ip);
        wcscpy(config->group, L"기타");
        config->priority = 5;
        return FALSE;
    }

    // Parse: "그룹,우선순위"
    wchar_t *comma = wcschr(value, L',');
    if (comma)
    {
        *comma = L'\0';
        wcscpy(config->group, value);
        config->priority = _wtoi(comma + 1);
    }
    else
    {
        wcscpy(config->group, value);
        config->priority = 5;
    }

    wcscpy(config->ip, ip);
    return TRUE;
}

// ============================================================================
// Set IP Group Configuration
// ============================================================================
BOOL SetIPGroupConfig(const wchar_t *configPath, const IPGroupConfig *config)
{
    if (!config)
        return FALSE;

    wchar_t value[256];
    swprintf(value, 256, L"%s,%d", config->group, config->priority);

    return WritePrivateProfileStringW(L"IPGroups", config->ip, value, configPath);
}

// ============================================================================
// Export Configuration as JSON
// ============================================================================
void ExportConfigJSON(const OutageConfig *config, wchar_t *jsonBuffer, int bufferSize)
{
    if (!config || !jsonBuffer)
        return;

    wchar_t *p = jsonBuffer;
    int remaining = bufferSize;

    // Start JSON
    p += swprintf(p, remaining, L"{\n  \"defaultThreshold\": %d,\n  \"groups\": [\n",
                  config->defaultThreshold);
    remaining = bufferSize - (int)(p - jsonBuffer);

    // Groups
    for (int i = 0; i < config->groupCount; i++)
    {
        p += swprintf(p, remaining,
                      L"    {\"name\": \"%s\", \"threshold\": %d}%s\n",
                      config->groups[i].groupName,
                      config->groups[i].thresholdSeconds,
                      (i < config->groupCount - 1) ? L"," : L"");
        remaining = bufferSize - (int)(p - jsonBuffer);
    }

    // End JSON
    swprintf(p, remaining, L"  ]\n}");
}

// ============================================================================
// Import Configuration from JSON
// ============================================================================
BOOL ImportConfigJSON(const wchar_t *jsonString, OutageConfig *config)
{
    // Simple JSON parsing (basic implementation)
    // For production, use a proper JSON parser

    if (!config || !jsonString)
        return FALSE;

    // This is a placeholder - full implementation would parse JSON properly
    return FALSE;
}