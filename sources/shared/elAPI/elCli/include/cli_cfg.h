/**
 * @file                cli_cfg.h
 * @brief               elCLI configuration types and constants.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * Defines elCliCfg_t. Pass config to elCliInit(cfg).
 * printFunc is required; output goes only through printFunc (e.g. wrap elUartSend).
 *
 * @note When pwdHashFunc is NULL, auth will always fail unless disabled.
 */

#pragma once

#ifdef CONFIG_ELAPI_CLI_EN

#include <stdint.h>

#include "cli_auth.h"

typedef void (*elCliPwdHashFunc_t)(const uint8_t *pwd, uint32_t len, uint8_t *hashOut);
typedef void (*elCliPrintFunc_t)(const char *buf, unsigned len);

typedef struct {
	elCliUser_t          users[ELCLI_USER_MAX_COUNT];
	unsigned int         signOutInactivityPeriod;
	elCliPwdHashFunc_t   pwdHashFunc;
	elCliPrintFunc_t     printFunc;  /**< Required; output goes only through this */
} elCliCfg_t;

const elCliCfg_t *elCliGetConfig(void);

void elCliPrintStr(const char *buf);
void elCliPrintCh(char ch);
void elCliPrintf(const char *fmt, ...);

#endif /* CONFIG_ELAPI_CLI_EN */
