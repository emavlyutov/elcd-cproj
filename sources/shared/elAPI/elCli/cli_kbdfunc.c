/**
 * @file                cli_kbdfunc.c
 * @brief               elCLI keyboard escape-sequence parser implementation.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * Parses VT100/ANSI escape sequences for arrow keys, F-keys, and special keys.
 *
 * @note Returns ELCLI_KBD_WAIT while sequence is incomplete.
 */

#ifdef CONFIG_ELAPI_CLI_EN

#include <stdbool.h>
#include <stddef.h>

#include "cli_kbdfunc.h"

#define ELCLI_KBD_CHARMAP_LEN		5

typedef struct {
	elCliKbdButton_t button;
	char charmap[ELCLI_KBD_CHARMAP_LEN];
	int charlen;
} elCliKbdBtnLayout_t;

static const elCliKbdBtnLayout_t elCliButtonsLayout[] = {
	{ELCLI_KBD_UP,    {0x1b, 0x5b, 0x41, 0x00, 0x00}, 3},
	{ELCLI_KBD_DOWN,  {0x1b, 0x5b, 0x42, 0x00, 0x00}, 3},
	{ELCLI_KBD_LEFT,  {0x1b, 0x5b, 0x44, 0x00, 0x00}, 3},
	{ELCLI_KBD_RIGHT, {0x1b, 0x5b, 0x43, 0x00, 0x00}, 3},
	{ELCLI_KBD_F1,    {0x1b, 0x5b, 0x31, 0x31, 0x7e}, 5},
	{ELCLI_KBD_F2,    {0x1b, 0x5b, 0x31, 0x32, 0x7e}, 5},
	{ELCLI_KBD_F3,    {0x1b, 0x5b, 0x31, 0x33, 0x7e}, 5},
	{ELCLI_KBD_F4,    {0x1b, 0x5b, 0x31, 0x34, 0x7e}, 5},
	{ELCLI_KBD_F5,    {0x1b, 0x5b, 0x31, 0x35, 0x7e}, 5},
	{ELCLI_KBD_F6,    {0x1b, 0x5b, 0x31, 0x37, 0x7e}, 5},
	{ELCLI_KBD_F7,    {0x1b, 0x5b, 0x31, 0x38, 0x7e}, 5},
	{ELCLI_KBD_F8,    {0x1b, 0x5b, 0x31, 0x39, 0x7e}, 5},
	{ELCLI_KBD_F9,    {0x1b, 0x5b, 0x32, 0x30, 0x7e}, 5},
	{ELCLI_KBD_F10,   {0x1b, 0x5b, 0x32, 0x31, 0x7e}, 5},
	{ELCLI_KBD_F11,   {0x1b, 0x5b, 0x32, 0x33, 0x7e}, 5},
	{ELCLI_KBD_F12,   {0x1b, 0x5b, 0x32, 0x34, 0x7e}, 5},
	{ELCLI_KBD_HOME,  {0x1b, 0x5b, 0x31, 0x7e, 0x00}, 4},
	{ELCLI_KBD_END,   {0x1b, 0x5b, 0x34, 0x7e, 0x00}, 4},
	{ELCLI_KBD_INS,   {0x1b, 0x5b, 0x32, 0x7e, 0x00}, 4},
	{ELCLI_KBD_DEL,   {0x1b, 0x5b, 0x33, 0x7e, 0x00}, 4},
	{ELCLI_KBD_PGUP,  {0x1b, 0x5b, 0x35, 0x7e, 0x00}, 4},
	{ELCLI_KBD_PGDN,  {0x1b, 0x5b, 0x36, 0x7e, 0x00}, 4},
};

/**
 * @brief   Parse RX byte as keyboard input; returns button or ELCLI_KBD_WAIT.
 */
elCliKbdButton_t elCliKbdRxByteHandle(char ch)
{
	static int index = 0;
	static char buf[ELCLI_KBD_CHARMAP_LEN] = {0};
	bool flag;
	size_t layoutCnt = sizeof(elCliButtonsLayout) / sizeof(elCliKbdBtnLayout_t);

	if (index) {
		if (ELCLI_KBD_CHARMAP_LEN == index) {
			index = 0;
			return ELCLI_KBD_OTHER;
		}
		buf[index++] = ch;
		for (size_t i = 0; i < layoutCnt; i++) {
			if (index == elCliButtonsLayout[i].charlen) {
				flag = true;
				for (int j = 0; j < index; j++) {
					if (buf[j] != elCliButtonsLayout[i].charmap[j]) {
						flag = false;
						break;
					}
				}
				if (flag) {
					index = 0;
					return elCliButtonsLayout[i].button;
				}
			}
		}
		return ELCLI_KBD_WAIT;
	}
	switch (ch) {
	case 0x03:
		return ELCLI_KBD_BREAK;
	case 0x09:
		return ELCLI_KBD_TAB;
	case 0x1b:
		buf[index++] = ch;
		return ELCLI_KBD_WAIT;
	}
	return ELCLI_KBD_NONE;
}

#endif /* CONFIG_ELAPI_CLI_EN */
