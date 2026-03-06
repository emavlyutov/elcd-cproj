/**
 * @file                cli.h
 * @brief               elCLI main public interface.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * Command-line interface for embedded systems. Pass config to elCliInit(cfg).
 * Call elCliRxCharProcess() from UART RX handler. Register handler via elIrqRegister
 * (elHAL) or elRtosIrqInstall (when OS_ELRTOS_EN); elIrqRegister delegates to
 * elRtosIrqInstall when elRTOS IRQ-managed is enabled.
 *
 * @note Requires CONFIG_ELAPI_CLI_EN.
 */

#pragma once

#ifdef CONFIG_ELAPI_CLI_EN

#include "cli_cfg.h"
#include "cli_auth.h"
#include "cli_charfunc.h"
#include "cli_cmdhandler.h"
#include "cli_apps.h"

#include "elUtils/elerrcode.h"
#include <stdint.h>

#ifndef ELCLI_CMD_OUTPUT_BUFFER_SIZE
#define ELCLI_CMD_OUTPUT_BUFFER_SIZE	4096
#endif
#ifndef ELCLI_CMD_QUEUE_DEPTH
#define ELCLI_CMD_QUEUE_DEPTH			4
#endif

/**
 * @brief   Initialize CLI with configuration.
 * @param   cfg   CLI configuration (printFunc required).
 * @return  EL_SUCCESS or error code.
 */
el_errcode_e elCliInit(const elCliCfg_t *cfg);

/**
 * @brief   Process single RX character. Call from UART RX IRQ handler.
 */
void         elCliRxCharProcess(uint8_t ch);

#if !defined(OS_ELRTOS_EN)
/**
 * @brief   Poll CLI (bare-metal). Call from main loop when !OS_ELRTOS_EN.
 * @param   msSinceLastCall   Milliseconds since last poll (for auth sign-out timer).
 */
void         elCliPoll(uint32_t msSinceLastCall);
#endif

#endif /* CONFIG_ELAPI_CLI_EN */
