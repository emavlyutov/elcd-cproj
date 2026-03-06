/**
 * @file                cli_cfg.c
 * @brief               elCLI configuration storage.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * Configuration storage. Config is set via elCliInit(cfg).
 */

#ifdef CONFIG_ELAPI_CLI_EN

#include <stdio.h>
#include <stdarg.h>

#include "cli_cfg.h"

#define ELCLI_PRINTF_BUF_SIZE    256

static const elCliCfg_t *elCliConfiguration = NULL;

void elCliCfgSet(const elCliCfg_t *cfg)
{
	elCliConfiguration = cfg;
}

/**
 * @brief   Get current CLI configuration.
 */
const elCliCfg_t *elCliGetConfig(void)
{
	return elCliConfiguration;
}

/**
 * @brief   Print string via printFunc only. No-op if printFunc is NULL.
 */
void elCliPrintStr(const char *buf)
{
	const elCliCfg_t *cfg = elCliGetConfig();
	if (!cfg || !cfg->printFunc || !buf)
		return;
	unsigned len = 0;
	while (buf[len])
		len++;
	if (len)
		cfg->printFunc(buf, len);
}

/**
 * @brief   Print single character via printFunc only. No-op if printFunc is NULL.
 */
void elCliPrintCh(char ch)
{
	const elCliCfg_t *cfg = elCliGetConfig();
	if (!cfg || !cfg->printFunc)
		return;
	cfg->printFunc(&ch, 1);
}

/**
 * @brief   Print formatted string via printFunc (printf-style).
 */
void elCliPrintf(const char *fmt, ...)
{
	va_list ap;
	char    buf[ELCLI_PRINTF_BUF_SIZE];

	va_start(ap, fmt);
	if (vsnprintf(buf, sizeof(buf), fmt, ap) > 0)
		elCliPrintStr(buf);
	va_end(ap);
}

#endif /* CONFIG_ELAPI_CLI_EN */
