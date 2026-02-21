/**
 * @file                cli_cmdhandler.c
 * @brief               elCLI command registration and execution implementation.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * Terminal context stack, command list, command interpreter.
 * Uses elRtosMutex for list protection, malloc/free for allocation.
 *
 * @note Ensure sufficient heap for command structures.
 */

#ifdef CONFIG_ELAPI_CLI_EN

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "elassert.h"
#include "cli_charfunc.h"
#include "cli_cmdhandler.h"

#if defined(OS_ELRTOS_EN)
#include "elrtos.h"
#endif

static int exitCmdInterpreter(char *wrBuf, size_t wrLen, const char *cmdStr);
static int doCmdInterpreter(char *wrBuf, size_t wrLen, const char *cmdStr);
static int helpCmdInterpreter(char *wrBuf, size_t wrLen, const char *cmdStr);

extern const elCliCommand_t signoutCmd;

static const elCliCommand_t doCmd = {
	"do",
	"do action from main terminal",
	"help do",
	doCmdInterpreter,
	-1,
	true
};
static const elCliCommand_t exitCmd = {
	"exit",
	"exit to previous terminal",
	"help exit",
	exitCmdInterpreter,
	0,
	true
};
static const elCliCommand_t helpCmd = {
	"help",
	"echo commands list or help for typed command",
	"help help",
	helpCmdInterpreter,
	-1,
	false
};

typedef struct elCliCmdList {
	const elCliCommand_t *cmdDef;
	struct elCliCmdList  *next;
} elCliCmdList_t;

typedef struct elCliTerminal {
	elCliCmdList_t      *cmdList;
	char                 historyBuf[ELCLI_HISTORY_RECORDS_COUNT][ELCLI_COMMAND_BUFLEN];
	unsigned int         histCount;
	unsigned int         histWrIdx;
	unsigned int         histRdIdx;
	struct elCliTerminal *prev;
} elCliTerminal_t;

static elCliTerminal_t  *curTerminal = NULL;
#if defined(OS_ELRTOS_EN)
static elRtosMutex_t     listMutex;
static bool              mutexInited = false;
#endif

/**
 * @brief   Register single command in current terminal.
 */
static int elCliRegisterCommand(const elCliCommand_t *cmdToReg)
{
	elCliCmdList_t *lastInList;
	elCliCmdList_t *newItem;
	int             ret = ELCLI_CMD_FAIL;

	CHECK_ASSERT_ARG_NOT_NULL(curTerminal);
	CHECK_ASSERT_ARG_NOT_NULL(cmdToReg);

	lastInList = curTerminal->cmdList;
	newItem = (elCliCmdList_t *)malloc(sizeof(elCliCmdList_t));
	CHECK_ASSERT_ARG_NOT_NULL(newItem);

	if (newItem) {
#if defined(OS_ELRTOS_EN)
		elRtosMutexTake(&listMutex, EL_RTOS_NEVER_TIMEOUT, false, false);
#endif
		newItem->cmdDef = cmdToReg;
		newItem->next = NULL;
		if (lastInList) {
			while (lastInList->next)
				lastInList = lastInList->next;
			lastInList->next = newItem;
		} else {
			curTerminal->cmdList = newItem;
		}
#if defined(OS_ELRTOS_EN)
		elRtosMutexGive(&listMutex, false, false);
#endif
		ret = ELCLI_CMD_OK;
	}
	return ret;
}

int elCliRegisterTerminal(const elCliCommand_t **cmdPtr, int cmdCount, bool isAdmin)
{
	int              status;
	elCliTerminal_t *oldTerm = curTerminal;

#if defined(OS_ELRTOS_EN)
	if (!mutexInited) {
		elRtosMutexInit(&listMutex, false, true);
		mutexInited = true;
	}
#endif

	curTerminal = (elCliTerminal_t *)malloc(sizeof(elCliTerminal_t));
	CHECK_ASSERT_ARG_NOT_NULL(curTerminal);
	memset(curTerminal, 0, sizeof(elCliTerminal_t));

	if (oldTerm) {
		curTerminal->prev = oldTerm;
		status = elCliRegisterCommand(&doCmd);
		CHECK_ASSERT_EXPR(status == ELCLI_CMD_OK);
		status = elCliRegisterCommand(&exitCmd);
		CHECK_ASSERT_EXPR(status == ELCLI_CMD_OK);
	}
	status = elCliRegisterCommand(&signoutCmd);
	CHECK_ASSERT_EXPR(status == ELCLI_CMD_OK);
	status = elCliRegisterCommand(&helpCmd);
	CHECK_ASSERT_EXPR(status == ELCLI_CMD_OK);

	for (int i = 0; i < cmdCount; i++) {
		if (NULL == cmdPtr)
			return ELCLI_CMD_FAIL;
		if (cmdPtr[i]->requireAdmin == isAdmin) {
			status = elCliRegisterCommand(cmdPtr[i]);
			CHECK_ASSERT_EXPR(status == ELCLI_CMD_OK);
		}
	}
	return ELCLI_CMD_OK;
}

void elCliUnRegisterTerminal(void)
{
	elCliCmdList_t     *cmdInList;
	elCliCmdList_t     *nextCmdInList;
	elCliTerminal_t    *prevTerm;

	CHECK_ASSERT_ARG_NOT_NULL(curTerminal);
	prevTerm = curTerminal->prev;
	nextCmdInList = curTerminal->cmdList;
	if (NULL == nextCmdInList)
		goto free_term;

#if defined(OS_ELRTOS_EN)
	elRtosMutexTake(&listMutex, EL_RTOS_NEVER_TIMEOUT, false, false);
#endif
	while (nextCmdInList) {
		cmdInList = nextCmdInList;
		nextCmdInList = cmdInList->next;
		free(cmdInList);
	}
	free(curTerminal);
#if defined(OS_ELRTOS_EN)
	elRtosMutexGive(&listMutex, false, false);
#endif
	curTerminal = prevTerm;
	return;

free_term:
	free(curTerminal);
	curTerminal = prevTerm;
}

void elCliUnRegisterAllTerminal(void)
{
	while (curTerminal != NULL)
		elCliUnRegisterTerminal();
}

/**
 * @brief   Process command at given terminal.
 */
static int elCliProcessCommandAtTerminal(elCliTerminal_t *term, const char *cmdInput, char *cmdOutput, size_t outputMaxLen)
{
	const elCliCmdList_t *cmd = NULL;
	int                   ret = ELCLI_CMD_OK;
	const char           *regCmdStr;
	size_t                cmdStrLen;
	elCliCmdList_t       *regCmds;

	if (NULL == term)
		return ELCLI_CMD_FAIL;

	regCmds = term->cmdList;
	for (cmd = regCmds; cmd != NULL; cmd = cmd->next) {
		regCmdStr = cmd->cmdDef->cmd;
		cmdStrLen = strlen(regCmdStr);
		if (0 == strncmp(cmdInput, regCmdStr, cmdStrLen)) {
			if ((cmdInput[cmdStrLen] == ' ') || (cmdInput[cmdStrLen] == '\0')) {
				if (cmd->cmdDef->expParamCount >= 0) {
					if (elCliGetNumberOfParameters(cmdInput) != cmd->cmdDef->expParamCount)
						ret = ELCLI_CMD_FAIL;
				}
				break;
			}
		}
	}

	if ((cmd != NULL) && (ret == ELCLI_CMD_FAIL)) {
		strncpy(cmdOutput, "Incorrect command parameter(s). Type "ANSI_COLOR_CYAN"help"ANSI_COLOR_RESET" to view a list of available commands.", outputMaxLen);
		cmd = NULL;
	} else if (cmd != NULL) {
		ret = cmd->cmdDef->interpreter(cmdOutput, outputMaxLen, cmdInput);
		if (ret == ELCLI_CMD_FAIL)
			cmd = NULL;
	} else {
		strncpy(cmdOutput, "Command not recognised.  Type "ANSI_COLOR_CYAN"help"ANSI_COLOR_RESET" to view a list of available commands.", outputMaxLen);
		ret = ELCLI_CMD_FAIL;
	}
	return ret;
}

int elCliProcessCommand(const char *cmdInput, char *cmdOutput, size_t outputMaxLen)
{
	return elCliProcessCommandAtTerminal(curTerminal, cmdInput, cmdOutput, outputMaxLen);
}

static int exitCmdInterpreter(char *wrBuf, size_t wrLen, const char *cmdStr)
{
	(void)wrBuf;
	(void)wrLen;
	(void)cmdStr;
	elCliUnRegisterTerminal();
	return ELCLI_CMD_FAIL;
}

static int doCmdInterpreter(char *wrBuf, size_t wrLen, const char *cmdStr)
{
	elCliTerminal_t *mainTerm;

	if (NULL == curTerminal)
		return ELCLI_CMD_FAIL;
	mainTerm = curTerminal;
	while (mainTerm->prev != NULL)
		mainTerm = mainTerm->prev;
	return elCliProcessCommandAtTerminal(mainTerm, cmdStr, wrBuf, wrLen);
}

static int helpCmdInterpreter(char *wrBuf, size_t wrLen, const char *cmdStr)
{
	int                   ret = ELCLI_CMD_FAIL;
	const elCliCmdList_t  *cmd = NULL;
	size_t                paramLen;
	char                 *param = (char *)elCliGetParameter(cmdStr, 1, &paramLen);
	elCliCmdList_t       *regCmds;

	if (NULL == curTerminal)
		return ELCLI_CMD_FAIL;
	regCmds = curTerminal->cmdList;

	if (param != NULL) {
		param[paramLen] = '\0';
		for (cmd = regCmds; cmd != NULL; cmd = cmd->next) {
			if (paramLen > 0 &&
			    strncmp(param, cmd->cmdDef->cmd, paramLen - 1) == 0) {
				strncpy(wrBuf, cmd->cmdDef->helpStr, wrLen);
				return ELCLI_CMD_FAIL;
			}
		}
		strncpy(wrBuf, "Command not recognised.  Type "ANSI_COLOR_CYAN"help"ANSI_COLOR_RESET" to view a list of available commands.", wrLen);
	} else {
		if (cmd == NULL)
			cmd = regCmds;
		snprintf(wrBuf, wrLen, ANSI_COLOR_CYAN"%-16s"ANSI_COLOR_RESET" - %s", cmd->cmdDef->cmd, cmd->cmdDef->descStr);
		cmd = cmd->next;
		ret = (cmd == NULL) ? ELCLI_CMD_FAIL : ELCLI_CMD_OK;
	}
	return ret;
}

#endif /* CONFIG_ELAPI_CLI_EN */
