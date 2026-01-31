/**
 * Config Module
 * Configuration file loading
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"

// ============================================================================
// Function Declarations
// ============================================================================

/**
 * Load config from single file
 * @param configFile Config file name (ping_config.ini or int_config.ini)
 */
void LoadConfigFromFile(const wchar_t *configFile);

/**
 * Load all config files
 * Loads both ping_config.ini and int_config.ini
 */
void LoadConfig(void);

#endif // CONFIG_H