/**
 * @file                cli_apps.h
 * @brief               elCLI sub-application (terminal mode) support.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * Allows running interactive sub-apps (e.g. vi-like) within the CLI context.
 *
 * @note Pass task handle, not pointer, to vTaskDelete.
 */

#pragma once

#ifdef CONFIG_ELAPI_CLI_EN

#include "cli_charfunc.h"
#include "cli_kbdfunc.h"

typedef struct {
	int executePeriod;
	int (*executeApp)(void *callBackRef);  /**< Return non-zero while running. */
	void *executeCallBackRef;
	int (*terminateApp)(void);
	int (*processChar)(char ch);           /**< Return non-zero if consumed. */
	int (*processKBD)(elCliKbdButton_t btn);
} elCliApp_t;

bool elCliAppIsRunning(void);
int elCliAppStart(elCliApp_t *app, void *callBackRef);
void elCliAppTerminate(void);
int elCliAppKeyProcess(elCliKbdButton_t button);
int elCliAppByteProcess(char ch);

#endif /* CONFIG_ELAPI_CLI_EN */
