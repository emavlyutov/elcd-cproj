/**
 * @file                platform_common.h
 * @brief               This header file declares platform initialization and application entry.
 *
 * @author              Eldar Mavlyutov
 * @date                2025-12-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * This module declares platform_init(), platform_deinit(), and application_start()
 * to be implemented by each project's platform_common.c.
 *
 * @note This file should be included by main.c and platform-specific sources.
 * @warning Do not modify this file directly unless you are the designated maintainer.
 */

int platform_init(void);
int platform_deinit(void);
int application_start(void);
