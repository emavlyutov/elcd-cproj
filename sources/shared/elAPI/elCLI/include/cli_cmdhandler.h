/**
 * @file                cli_cmdhandler.h
 * @brief               elCLI command registration and execution.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * Terminal context stack, command list, and command interpreter.
 * When OS_ELRTOS_EN: uses elRTOS mutex. Else: no protection (single-threaded).
 */

#pragma once

#ifdef CONFIG_ELAPI_CLI_EN

#include <stdbool.h>
#include <stddef.h>

#define ELCLI_COMMAND_BUFLEN            256
#define ELCLI_COMMAND_MAXLEN            16
#define ELCLI_HISTORY_RECORDS_COUNT     16

#define ELCLI_CMD_OK    1
#define ELCLI_CMD_FAIL  0

typedef int (*elCliCommandCallback_t)(char *wrBuf, size_t wrLen, const char *cmdStr);

typedef struct {
	const char                   cmd[ELCLI_COMMAND_MAXLEN];
	const char *const            descStr;
	const char *const            helpStr;
	const elCliCommandCallback_t interpreter;
	int8_t                       expParamCount;
	bool                         requireAdmin;
} elCliCommand_t;

/**
 * @brief   Register terminal with optional command set.
 * @param  cmdPtr     Array of commands to add, or NULL.
 * @param  cmdCount   Number of commands in cmdPtr.
 * @param  isAdmin    True if admin terminal.
 * @return ELCLI_CMD_OK on success, ELCLI_CMD_FAIL otherwise.
 */
int elCliRegisterTerminal(const elCliCommand_t **cmdPtr, int cmdCount, bool isAdmin);

/**
 * @brief   Unregister current terminal and return to previous.
 */
void elCliUnRegisterTerminal(void);

/**
 * @brief   Unregister all terminals.
 */
void elCliUnRegisterAllTerminal(void);

/**
 * @brief   Process command string and fill output buffer.
 * @param  cmdInput    Input command string.
 * @param  cmdOutput   Output buffer.
 * @param  outputMaxLen  Output buffer size.
 * @return ELCLI_CMD_OK if more output follows, ELCLI_CMD_FAIL when done.
 */
int elCliProcessCommand(const char *cmdInput, char *cmdOutput, size_t outputMaxLen);

#endif /* CONFIG_ELAPI_CLI_EN */
