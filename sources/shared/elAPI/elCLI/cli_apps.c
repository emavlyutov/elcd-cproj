/**
 * @file                cli_apps.c
 * @brief               elCLI sub-application (terminal mode) implementation.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * Runs interactive sub-apps within CLI. Uses elRTOS for task management.
 *
 * @note Sub-app executeApp runs in RTOS task context.
 */

#ifdef CONFIG_ELAPI_CLI_EN

#include "elassert.h"
#include "elrtos.h"

#include "cli_apps.h"

static elRtosTask_t  appTask;
static elCliApp_t   *currentApp = NULL;

static long elCliAppTaskHandler(void *args);

/**
 * @brief   Return true if a sub-app is currently running.
 */
bool elCliAppIsRunning(void)
{
	return (currentApp != NULL);
}

/**
 * @brief   Start sub-app; creates RTOS task for executeApp.
 * @return  Non-zero on success, 0 on failure.
 */
int elCliAppStart(elCliApp_t *app, void *callBackRef)
{
	CHECK_RETURN_ARG_NOT_NULL(app, 0);
	CHECK_RETURN_EXPR(app->executeApp != NULL, 0);

	app->executeCallBackRef = callBackRef;
	currentApp = app;
	if (elRtosTaskCreate(&appTask, "CLIAPP", elCliAppTaskHandler, NULL, 1, NULL, 4000U) != EL_SUCCESS)
		return 0;
	return 1;
}

/**
 * @brief   Terminate running sub-app and delete its task.
 */
void elCliAppTerminate(void)
{
	if (currentApp != NULL) {
		currentApp->executeCallBackRef = NULL;
		if (currentApp->terminateApp != NULL)
			currentApp->terminateApp();
		elRtosTaskDelete(&appTask);
		currentApp = NULL;
	}
}

/**
 * @brief   Process keyboard button; BREAK terminates app.
 * @return  Non-zero if key was handled.
 */
int elCliAppKeyProcess(elCliKbdButton_t button)
{
	if (ELCLI_KBD_BREAK == button) {
		elCliAppTerminate();
		return 0;
	}
	if (NULL == currentApp)
		return 0;
	if (currentApp->processKBD != NULL)
		currentApp->processKBD(button);
	elRtosTaskDelete(&appTask);
	currentApp = NULL;
	return 1;
}

/**
 * @brief   Process single character in sub-app mode.
 * @return  Non-zero if consumed.
 */
int elCliAppByteProcess(char ch)
{
	if (NULL == currentApp)
		return 0;
	if (currentApp->processChar != NULL)
		return currentApp->processChar(ch);
	return 1;
}

static long elCliAppTaskHandler(void *args)
{
	int running = 0;

	(void)args;
	for (;;) {
		if (currentApp != NULL) {
			if (currentApp->executeApp != NULL)
				running = currentApp->executeApp(currentApp->executeCallBackRef);
			if (running)
				elRtosDelay(currentApp->executePeriod);
			else
				elRtosDelay(1);
		} else {
			elRtosDelay(1);
		}
	}
	return 0;
}

#endif /* CONFIG_ELAPI_CLI_EN */
