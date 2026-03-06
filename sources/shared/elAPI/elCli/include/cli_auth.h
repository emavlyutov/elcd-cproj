/**
 * @file                cli_auth.h
 * @brief               elCLI authentication module public interface.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * Provides user authentication (login/password) and sign-out for the CLI.
 * Password hashing is delegated via pwdHashFunc in elCliCfg_t.
 *
 * @note Pass config with pwdHashFunc set to elCliInit() when auth is used.
 * @warning pwdHashFunc must be set by application when CONFIG_ELAPI_CLI_AUTH is used.
 */

#pragma once

#ifdef CONFIG_ELAPI_CLI_EN

#include <stdbool.h>
#include <stdint.h>

#define ELCLI_USER_MAX_COUNT				4
#define ELCLI_AUTH_UNAME_MAXLEN			16
#define ELCLI_AUTH_PWD_MAXLEN				16
#define ELCLI_AUTH_SIGNOUT_TIME			60
#define ELCLI_PWD_HASH_BYTELEN			16

typedef struct {
	char uName[ELCLI_AUTH_UNAME_MAXLEN];
	uint8_t pwdHash[ELCLI_PWD_HASH_BYTELEN];
	bool isAdmin;
} elCliUser_t;

void elCliAuthInit(void);
bool elCliGetAuthStatus(void);
void elCliUpdateAuthStatus(void);
void elCliPrintAuthString(void);
char elCliAuthByteProcess(char ch);

#if !defined(OS_ELRTOS_EN)
/**
 * @brief   Poll auth (bare-metal). Call from elCliPoll.
 * @param   msSinceLastCall   Milliseconds since last poll (sign-out timer).
 */
void elCliAuthPoll(uint32_t msSinceLastCall);
#endif

#endif /* CONFIG_ELAPI_CLI_EN */
