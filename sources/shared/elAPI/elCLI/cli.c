/**
 * @file                cli.c
 * @brief               elCLI main implementation and RX processing.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * When OS_ELRTOS_EN: queue + task. Else: ring buffer + elCliPoll from main loop.
 * Call elCliRxCharProcess from UART RX. Output via printFunc only.
 *
 * @note Requires CONFIG_ELAPI_CLI_EN.
 */

#ifdef CONFIG_ELAPI_CLI_EN

#include <string.h>
#include <stdint.h>

#include "elassert.h"
#include "elerrcode.h"

#include "cli.h"
#include "cli_cfg.h"

extern void elCliCfgSet(const elCliCfg_t *cfg);  /* internal, in cli_cfg.c */

#if defined(OS_ELRTOS_EN)
#include "elrtos.h"

static elRtosQueue_t cmdQueue;
static elRtosTask_t  cliTask;

static long elCliTaskHandler(void *arg);
#else
static char  pendingCmd[ELCLI_COMMAND_BUFLEN];
static bool  pendingCmdReady = false;
#endif

/**
 * @brief   Initialize CLI with configuration.
 * @param   cfg   CLI configuration (printFunc required).
 * @return  EL_SUCCESS or error code.
 */
el_errcode_e elCliInit(const elCliCfg_t *cfg)
{
	CHECK_RETURN_ARG_NOT_NULL(cfg, EL_ERR_EINVAL);
	CHECK_RETURN_ARG_NOT_NULL(cfg->printFunc, EL_ERR_EINVAL);

	elCliCfgSet(cfg);

#if defined(OS_ELRTOS_EN)
	{
		el_errcode_e err;
		err = elRtosQueueInit(&cmdQueue, ELCLI_CMD_QUEUE_DEPTH, ELCLI_COMMAND_BUFLEN);
		CHECK_RETURN_EXPR(err == EL_SUCCESS, EL_ERR_CLI_QUEUE);

		err = elRtosTaskCreate(&cliTask, "CLITASK", elCliTaskHandler, NULL, 1, NULL, 4000U);
		CHECK_RETURN_EXPR(err == EL_SUCCESS, EL_ERR_CLI_TASK);
	}
#else
	pendingCmdReady = false;
	memset(pendingCmd, 0, sizeof(pendingCmd));
#endif

	elCliPrintStr("\r\n\r\n");
	elCliPrintStr("Welcome to ElCyberDev command line interface\r\n"
		"Developed by "ANSI_COLOR_CYAN"ElCyberDev"ANSI_COLOR_RESET"\r\n"
		"mailto: "ANSI_COLOR_CYAN"elcyberdev@gmail.com"ANSI_COLOR_RESET"\r\n"
		"All rights reserved (2025)\r\n\r\n");

	elCliAuthInit();
	return EL_SUCCESS;
}

/**
 * @brief   Process single RX character: auth flow or command/keyboard dispatch.
 */
void elCliRxCharProcess(uint8_t ch)
{
	static int            byteIndex = 0;
	static char           uartRxCmdBuffer[ELCLI_COMMAND_BUFLEN];
	char                  outChar;
	elCliKbdButton_t      kbdButton;

#if defined(OS_ELRTOS_EN)
	bool         fromIrq = true;
	el_errcode_e err;
#endif

	if (elCliGetAuthStatus()) {
		elCliUpdateAuthStatus();
		kbdButton = elCliKbdRxByteHandle((char)ch);
		if (elCliAppIsRunning()) {
			if (ELCLI_KBD_WAIT == kbdButton)
				return;
			if (!((ELCLI_KBD_NONE == kbdButton) ? elCliAppByteProcess((char)ch) : elCliAppKeyProcess(kbdButton)))
				elCliAppTerminate();
			return;
		}
		switch (kbdButton) {
		case ELCLI_KBD_NONE:
			outChar = elCliRxByteHandle((char)ch, uartRxCmdBuffer, ELCLI_COMMAND_BUFLEN, &byteIndex, ELCLI_STRING_COMMAND);
			switch (outChar) {
			case 0:
				return;
			case '\n':
				if (0 != byteIndex) {
#if defined(OS_ELRTOS_EN)
					err = elRtosQueueWrite(&cmdQueue, uartRxCmdBuffer, 0, false, fromIrq);
					if (err == EL_SUCCESS) {
						memset(uartRxCmdBuffer, 0, sizeof(uartRxCmdBuffer));
						byteIndex = 0;
						elCliPrintStr("\r\n");
					}
#else
					if (!pendingCmdReady) {
						memcpy(pendingCmd, uartRxCmdBuffer, sizeof(pendingCmd));
						pendingCmdReady = true;
						memset(uartRxCmdBuffer, 0, sizeof(uartRxCmdBuffer));
						byteIndex = 0;
						elCliPrintStr("\r\n");
					}
#endif
				}
				return;
			default:
				elCliPrintCh(outChar);
				break;
			}
			break;
		case ELCLI_KBD_TAB:
		case ELCLI_KBD_WAIT:
			break;
		default:
			break;
		}
	} else {
		outChar = elCliAuthByteProcess((char)ch);
		if (0 != outChar) {
			if ('\n' == outChar)
				elCliPrintStr("\r\n");
			else
				elCliPrintCh(outChar);
		}
	}
}

#if defined(OS_ELRTOS_EN)
static long elCliTaskHandler(void *arg)
{
	char   cmdInput[ELCLI_COMMAND_BUFLEN];
	char   cmdOutput[ELCLI_CMD_OUTPUT_BUFFER_SIZE];
	int    moreData;

	(void)arg;
	for (;;) {
		if (elRtosQueueRead(&cmdQueue, cmdInput, EL_RTOS_NEVER_TIMEOUT, false) == EL_SUCCESS) {
			if (elCliGetAuthStatus()) {
				do {
					moreData = elCliProcessCommand(cmdInput, cmdOutput, ELCLI_CMD_OUTPUT_BUFFER_SIZE);
					if (moreData)
						elCliPrintStr("\r\n");
					elCliPrintStr(cmdOutput);
					memset(cmdOutput, 0, sizeof(cmdOutput));
				} while (moreData);
				elCliPrintAuthString();
			}
		}
	}
	return 0;
}

#else /* !OS_ELRTOS_EN */

static void elCliProcessPendingCommand(void)
{
	char cmdOutput[ELCLI_CMD_OUTPUT_BUFFER_SIZE];
	int  moreData;

	if (!pendingCmdReady || !elCliGetAuthStatus())
		return;
	memset(cmdOutput, 0, sizeof(cmdOutput));
	do {
		moreData = elCliProcessCommand(pendingCmd, cmdOutput, ELCLI_CMD_OUTPUT_BUFFER_SIZE);
		if (moreData)
			elCliPrintStr("\r\n");
		elCliPrintStr(cmdOutput);
		memset(cmdOutput, 0, sizeof(cmdOutput));
	} while (moreData);
	pendingCmdReady = false;
	elCliPrintAuthString();
}

void elCliPoll(uint32_t msSinceLastCall)
{
	elCliAuthPoll(msSinceLastCall);
	elCliProcessPendingCommand();
}

#endif /* OS_ELRTOS_EN */

#endif /* CONFIG_ELAPI_CLI_EN */
