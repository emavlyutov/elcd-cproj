/**
 * @file                main.c
 * @brief               Application entry point and main loop.
 *
 * @author              Eldar Mavlyutov
 * @date                2025-12-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * This module provides the main() entry point: platform_init(), application_start(),
 * and optionally elRtosStartScheduler() when RTOS is enabled.
 *
 * @note This file is the primary application entry for ElVPN projects.
 * @warning Do not modify this file directly unless you are the designated maintainer.
 */

#include "platform_common.h"
#ifdef OS_ELRTOS_EN
#include "elrtos.h"
#endif

int main(void) {
	platform_init();
	application_start();
#ifdef OS_ELRTOS_EN
	elRtosStartScheduler();
#endif
	platform_deinit();
	while(42);
	return 0;
}
