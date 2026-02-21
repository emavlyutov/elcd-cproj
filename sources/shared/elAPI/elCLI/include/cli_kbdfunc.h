/**
 * @file                cli_kbdfunc.h
 * @brief               elCLI keyboard escape-sequence handling.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * Parses ANSI escape sequences (arrow keys, F-keys, etc.) from serial input.
 *
 * @note Supports common VT100-style sequences.
 */

#pragma once

#ifdef CONFIG_ELAPI_CLI_EN

typedef enum {
	ELCLI_KBD_NONE,
	ELCLI_KBD_WAIT,
	ELCLI_KBD_TAB,
	ELCLI_KBD_UP,
	ELCLI_KBD_DOWN,
	ELCLI_KBD_LEFT,
	ELCLI_KBD_RIGHT,
	ELCLI_KBD_ESCAPE,
	ELCLI_KBD_BREAK,
	ELCLI_KBD_F1,
	ELCLI_KBD_F2,
	ELCLI_KBD_F3,
	ELCLI_KBD_F4,
	ELCLI_KBD_F5,
	ELCLI_KBD_F6,
	ELCLI_KBD_F7,
	ELCLI_KBD_F8,
	ELCLI_KBD_F9,
	ELCLI_KBD_F10,
	ELCLI_KBD_F11,
	ELCLI_KBD_F12,
	ELCLI_KBD_HOME,
	ELCLI_KBD_END,
	ELCLI_KBD_INS,
	ELCLI_KBD_DEL,
	ELCLI_KBD_PGUP,
	ELCLI_KBD_PGDN,
	ELCLI_KBD_OTHER
} elCliKbdButton_t;

elCliKbdButton_t elCliKbdRxByteHandle(char ch);

#endif /* CONFIG_ELAPI_CLI_EN */
