/**
 * Network Module
 * ICMP ping and target monitoring
 */

#ifndef NETWORK_H
#define NETWORK_H

#include "types.h"

// ============================================================================
// Function Declarations
// ============================================================================

/**
 * Execute ICMP ping
 * @param ip Target IP address
 * @param latency Output latency in milliseconds
 * @return TRUE if successful
 */
BOOL DoPing(const wchar_t *ip, DWORD *latency);

/**
 * Update target statistics
 * @param target Target to update
 * @param success Ping success
 * @param latency Latency value
 */
void UpdateTarget(IPTarget *target, BOOL success, DWORD latency);

/**
 * Build JSON data for web dashboard
 * @param buffer Output buffer
 * @param bufferSize Buffer size
 */
void BuildJsonData(wchar_t *buffer, size_t bufferSize);

/**
 * Send data to web view (write JSON file)
 */
void SendDataToWebView(void);

/**
 * Monitoring thread function
 * @param lpParam Thread parameter (unused)
 * @return Thread exit code
 */
DWORD WINAPI MonitoringThread(LPVOID lpParam);

/**
 * Start monitoring thread
 */
void StartMonitoring(void);

/**
 * Stop monitoring thread
 */
void StopMonitoring(void);

#endif // NETWORK_H