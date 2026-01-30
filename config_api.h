/**
 * Configuration API for Ping Monitor v2.6
 * Web UI configuration management
 */

#ifndef CONFIG_API_H
#define CONFIG_API_H

#include <windows.h>

// ============================================================================
// Constants
// ============================================================================
#define MAX_GROUP_NAME 64
#define MAX_GROUPS 20

// ============================================================================
// Structures
// ============================================================================

typedef struct
{
    wchar_t groupName[MAX_GROUP_NAME];
    int thresholdSeconds;
} GroupThreshold;

typedef struct
{
    wchar_t ip[64];
    wchar_t name[128];
    wchar_t group[MAX_GROUP_NAME];
    int priority;
} IPGroupConfig;

typedef struct
{
    int defaultThreshold;
    GroupThreshold groups[MAX_GROUPS];
    int groupCount;
} OutageConfig;

// ============================================================================
// Function Declarations
// ============================================================================

/**
 * Load configuration from ping_config.ini
 */
BOOL LoadOutageConfiguration(const wchar_t *configPath, OutageConfig *config);

/**
 * Save configuration to ping_config.ini
 */
BOOL SaveOutageConfiguration(const wchar_t *configPath, const OutageConfig *config);

/**
 * Get IP group configuration
 */
BOOL GetIPGroupConfig(const wchar_t *configPath, const wchar_t *ip, IPGroupConfig *config);

/**
 * Set IP group configuration
 */
BOOL SetIPGroupConfig(const wchar_t *configPath, const IPGroupConfig *config);

/**
 * Export configuration as JSON
 */
void ExportConfigJSON(const OutageConfig *config, wchar_t *jsonBuffer, int bufferSize);

/**
 * Import configuration from JSON
 */
BOOL ImportConfigJSON(const wchar_t *jsonString, OutageConfig *config);

#endif // CONFIG_API_H