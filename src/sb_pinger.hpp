#pragma once

#include <stdbool.h>

/**
 * @brief Check reachability of a target IP using ICMP ping.
 *
 * @param target_ip The IP address string (e.g., "192.168.1.1")
 * @return true if ping is successful, false otherwise
 */
bool sb_pinger_check(const char *target_ip);
