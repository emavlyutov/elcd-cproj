/**
 * @file                cli_auth.c
 * @brief               elCLI authentication implementation.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * Login/password flow, sign-out timer, credential checking.
 * Uses elCliPrintStr/elCliPrintf (printFunc only). Requires elRTOS task.
 *
 * @note pwdHashFunc must be set in elCliCfg_t when auth is used.
 */

#ifdef CONFIG_ELAPI_CLI_EN

#include <string.h>
#include <stdint.h>

#include "elassert.h"

#include "cli_auth.h"
#include "cli_apps.h"
#include "cli_charfunc.h"
#include "cli_cmdhandler.h"
#include "cli_cfg.h"

#if defined(OS_ELRTOS_EN)
#include "elrtos.h"
#define ELCLI_AUTH_TASK_DELAY_MS    1000
#endif

typedef enum {
	AUTH_UNAME,
	AUTH_PWD,
	AUTH_CHECK,
	AUTH_COMPLETE
} elCliAuthState_t;

static elCliAuthState_t authState = AUTH_UNAME;
static int              signoutPeriod = 0;
static bool             authIsAdmin = false;
static char             authUname[ELCLI_AUTH_UNAME_MAXLEN + 1];
static char             authPwd[ELCLI_AUTH_PWD_MAXLEN + 1];

static int signoutCmdInterpreter(char *wrBuf, size_t wrLen, const char *cmdStr);
static bool elCliAuthCheckAuthData(void);
static void elCliAuthSignIn(void);
static void elCliAuthSignOut(const char *reason);

const elCliCommand_t signoutCmd = {
	"signout",
	"sign out and close terminal session",
	"signout description",
	signoutCmdInterpreter,
	0,
	true
};

#if defined(OS_ELRTOS_EN)
static elRtosTask_t authTask;
static long elCliAuthTaskHandler(void *arg);
#endif

static void elCliPrintAuthWelcome(void)
{
	elCliPrintStr(ANSI_COLOR_RED "Authorization required" ANSI_COLOR_RESET "\r\n");
	elCliPrintStr(ANSI_COLOR_BG_WHITE ANSI_COLOR_BLACK "login:" ANSI_COLOR_RESET);
}

/**
 * @brief   Initialize CLI auth. When OS_ELRTOS_EN: start sign-out timer task.
 */
void elCliAuthInit(void)
{
#if defined(OS_ELRTOS_EN)
	elRtosTaskCreate(&authTask, "CLIAUTH", elCliAuthTaskHandler, NULL, 1, NULL, 4000U);
#endif
	elCliPrintAuthWelcome();
}

#if defined(OS_ELRTOS_EN)
static long elCliAuthTaskHandler(void *arg)
{
	(void)arg;
	for (;;) {
		elRtosDelay(ELCLI_AUTH_TASK_DELAY_MS);
		if (AUTH_CHECK == authState) {
			if (elCliAuthCheckAuthData()) {
				authState = AUTH_COMPLETE;
				elCliUpdateAuthStatus();
				elCliAuthSignIn();
				elCliPrintAuthString();
			} else {
				authState = AUTH_UNAME;
				memset(authUname, 0, ELCLI_AUTH_UNAME_MAXLEN);
				memset(authPwd, 0, ELCLI_AUTH_PWD_MAXLEN);
				elCliPrintStr("Incorrect login or password\r\n");
				elCliPrintAuthWelcome();
			}
		} else if (AUTH_COMPLETE == authState) {
			if (0 == signoutPeriod)
				elCliAuthSignOut("Inactivity period exceeded");
			else
				signoutPeriod--;
		}
	}
	return 0;
}
#else /* !OS_ELRTOS_EN */
void elCliAuthPoll(uint32_t msSinceLastCall)
{
	static uint32_t accumMs = 0;

	accumMs += msSinceLastCall;
	if (AUTH_CHECK == authState) {
		if (elCliAuthCheckAuthData()) {
			authState = AUTH_COMPLETE;
			elCliUpdateAuthStatus();
			elCliAuthSignIn();
			elCliPrintAuthString();
		} else {
			authState = AUTH_UNAME;
			memset(authUname, 0, ELCLI_AUTH_UNAME_MAXLEN);
			memset(authPwd, 0, ELCLI_AUTH_PWD_MAXLEN);
			elCliPrintStr("Incorrect login or password\r\n");
			elCliPrintAuthWelcome();
		}
		accumMs = 0;
		return;
	}
	if (AUTH_COMPLETE != authState) {
		accumMs = 0;
		return;
	}
	while (accumMs >= 1000U) {
		accumMs -= 1000U;
		if (0 == signoutPeriod) {
			elCliAuthSignOut("Inactivity period exceeded");
			accumMs = 0;
			return;
		}
		signoutPeriod--;
	}
}
#endif /* OS_ELRTOS_EN */

/**
 * @brief   Return true if user is authenticated.
 */
bool elCliGetAuthStatus(void)
{
	return (authState == AUTH_COMPLETE);
}

/**
 * @brief   Update sign-out timer from config.
 */
void elCliUpdateAuthStatus(void)
{
	CHECK_ASSERT_ARG_NOT_NULL(elCliGetConfig());
	signoutPeriod = (int)elCliGetConfig()->signOutInactivityPeriod;
	if (0 == signoutPeriod)
		signoutPeriod = ELCLI_AUTH_SIGNOUT_TIME;
}

/**
 * @brief   Print prompt (username + # or >).
 */
void elCliPrintAuthString(void)
{
	if (AUTH_COMPLETE == authState)
		elCliPrintf("\r\n" ANSI_COLOR_BG_WHITE ANSI_COLOR_BLACK "%s%c" ANSI_COLOR_RESET, authUname, authIsAdmin ? '#' : '>');
}

/**
 * @brief   Process single byte in auth flow (login/password).
 * @return  Character to echo, or 0 for none, '\n' for newline.
 */
char elCliAuthByteProcess(char ch)
{
	static int authIndex = 0;
	char       outChar = 0;

	switch (authState) {
	case AUTH_CHECK:
	case AUTH_COMPLETE:
		break;
	default:
		if (AUTH_UNAME == authState)
			outChar = elCliRxByteHandle(ch, authUname, ELCLI_AUTH_UNAME_MAXLEN, &authIndex, ELCLI_STRING_UNAME);
		else
			outChar = elCliRxByteHandle(ch, authPwd, ELCLI_AUTH_PWD_MAXLEN, &authIndex, ELCLI_STRING_PWD);
		switch (outChar) {
		case 0:
			break;
		case '\n':
			authIndex = 0;
			if (AUTH_UNAME == authState) {
				authState = AUTH_PWD;
				elCliPrintStr("\r\n" ANSI_COLOR_BG_WHITE ANSI_COLOR_BLACK "password:" ANSI_COLOR_RESET);
				return 0;
			}
			authState = AUTH_CHECK;
			return '\n';
		default:
			break;
		}
		break;
	}
	return outChar;
}

static bool elCliAuthCheckAuthData(void)
{
	uint8_t  hash[ELCLI_PWD_HASH_BYTELEN] = {0};
	uint32_t len = (uint32_t)strlen(authPwd);

	CHECK_ASSERT_ARG_NOT_NULL(elCliGetConfig());
	if (!elCliGetConfig()->pwdHashFunc)
		return false;
	elCliGetConfig()->pwdHashFunc((const uint8_t *)authPwd, len, hash);
	for (int i = 0; i < ELCLI_USER_MAX_COUNT; i++) {
		if ((0 == memcmp(hash, elCliGetConfig()->users[i].pwdHash, ELCLI_PWD_HASH_BYTELEN)) &&
		    (0 == memcmp(authUname, elCliGetConfig()->users[i].uName, ELCLI_AUTH_UNAME_MAXLEN))) {
			authIsAdmin = elCliGetConfig()->users[i].isAdmin;
			return true;
		}
	}
	return false;
}

static void elCliAuthSignIn(void)
{
	elCliPrintf("Authorization complete (" ANSI_COLOR_MAGENTA "%s" ANSI_COLOR_RESET ")\r\n", authIsAdmin ? "administrator" : "user");
	elCliRegisterTerminal(NULL, 0, authIsAdmin);
}

static void elCliAuthSignOut(const char *reason)
{
	authState = AUTH_UNAME;
	memset(authUname, 0, ELCLI_AUTH_UNAME_MAXLEN);
	memset(authPwd, 0, ELCLI_AUTH_PWD_MAXLEN);
	if (elCliAppIsRunning())
		elCliAppTerminate();
	elCliUnRegisterAllTerminal();
	if (NULL == reason)
		elCliPrintStr("\r\nSign out\r\n");
	else
		elCliPrintf("\r\nSign out (%s)\r\n", reason);
	elCliPrintAuthWelcome();
}

static int signoutCmdInterpreter(char *wrBuf, size_t wrLen, const char *cmdStr)
{
	(void)wrBuf;
	(void)wrLen;
	(void)cmdStr;
	elCliAuthSignOut(NULL);
	return ELCLI_CMD_FAIL;
}

#endif /* CONFIG_ELAPI_CLI_EN */
