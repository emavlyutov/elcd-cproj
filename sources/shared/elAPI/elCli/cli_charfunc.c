/**
 * @file                cli_charfunc.c
 * @brief               elCLI character handling and validation implementation.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * Character validation, RX byte handling, parameter parsing, and format validators
 * (MAC, IP, server address, integer, date, time).
 *
 * @note Uses strlen() for special-chars length; sizeof(ptr) would be wrong.
 */

#ifdef CONFIG_ELAPI_CLI_EN

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "cli_charfunc.h"

static bool elCliCheckCharIsValid(char ch, bool allowLower, bool allowUpper, bool allowDigits, const char *specialChars)
{
	if (allowLower && (ch >= 'a') && (ch <= 'z'))
		return true;
	if (allowUpper && (ch >= 'A') && (ch <= 'Z'))
		return true;
	if (allowDigits && (ch >= '0') && (ch <= '9'))
		return true;
	if (NULL != specialChars && strchr(specialChars, ch) != NULL)
		return true;
	return false;
}

static char elCliCheckCharValidByType(char ch, elCliStringType_t type)
{
	switch (type) {
	case ELCLI_STRING_COMMAND:
		if (elCliCheckCharIsValid(ch, true, true, true, " `~!@#$%^&*()_-=+{}[];:.,<>\\/?\'\""))
			return ch;
		break;
	case ELCLI_STRING_UNAME:
		if (elCliCheckCharIsValid(ch, true, true, true, "_"))
			return ch;
		break;
	case ELCLI_STRING_PWD:
		if (elCliCheckCharIsValid(ch, true, true, true, " `~!@#$%^&*()_-=+{}[];:.,<>\\/?\'\""))
			return '*';
		break;
	}
	return 0;
}

/**
 * @brief   Handle single RX byte: echo, backspace, newline.
 * @return  Character to echo, '\n' for newline, or 0 for none.
 */
char elCliRxByteHandle(char ch, char *buffer, int bufferLen, int *byteIndex, elCliStringType_t type)
{
	char outChar;

	if (('\r' == ch) || ('\n' == ch)) {
		if (0 != *byteIndex)
			return '\n';
	} else if ('\177' == ch) {
		if (0 != *byteIndex) {
			buffer[--(*byteIndex)] = 0;
			return ch;
		}
	} else {
		if (*byteIndex < bufferLen) {
			outChar = elCliCheckCharValidByType(ch, type);
			if (outChar) {
				buffer[(*byteIndex)++] = ch;
				return outChar;
			}
		}
	}
	return 0;
}

/**
 * @brief   Count space-separated parameters in command string.
 * @param   cmdStr  Command string (may include leading command name).
 * @return  Parameter count.
 */
int8_t elCliGetNumberOfParameters(const char *cmdStr)
{
	int8_t paramCount = 0;
	bool lastWasSpace = false;

	while (*cmdStr != '\0') {
		if (*cmdStr == ' ') {
			if (!lastWasSpace) {
				paramCount++;
				lastWasSpace = true;
			}
		} else {
			lastWasSpace = false;
		}
		cmdStr++;
	}
	if (lastWasSpace)
		paramCount--;
	return paramCount;
}

/**
 * @brief   Get nth space-separated parameter from command string.
 * @param   cmdStr      Command string.
 * @param   wantParam   Zero-based parameter index.
 * @param   paramLen    Output: length of parameter string.
 * @return  Pointer to parameter, or NULL if not found.
 */
const char *elCliGetParameter(const char *cmdStr, unsigned wantParam, size_t *paramLen)
{
	unsigned paramsFound = 0;
	const char *paramStr = NULL;

	*paramLen = 0;
	while (paramsFound < wantParam) {
		while ((*cmdStr != '\0') && (*cmdStr != ' '))
			cmdStr++;
		while ((*cmdStr != '\0') && (*cmdStr == ' '))
			cmdStr++;
		if (*cmdStr != '\0') {
			paramsFound++;
			if (paramsFound == wantParam) {
				paramStr = cmdStr;
				while ((*cmdStr != '\0') && (*cmdStr != ' ')) {
					(*paramLen)++;
					cmdStr++;
				}
				if (*paramLen == 0)
					paramStr = NULL;
				break;
			}
		} else {
			break;
		}
	}
	return paramStr;
}

typedef struct {
	uint32_t val;
	uint32_t valMin;
	uint32_t valMax;
	char     separator;
	bool     isHex;
} elCliMask_t;

static bool elCliValidateByMask(const char *str, elCliMask_t *mask)
{
	int maskIndex = 0;

	for (; *str; str++) {
		if (mask[maskIndex].isHex) {
			if ((*str >= '0') && (*str <= '9')) {
				mask[maskIndex].val *= 16;
				mask[maskIndex].val += (uint32_t)(*str - '0');
				continue;
			}
			if ((*str >= 'A') && (*str <= 'F')) {
				mask[maskIndex].val *= 16;
				mask[maskIndex].val += (uint32_t)(*str - 'A' + 10);
				continue;
			}
			if ((*str >= 'a') && (*str <= 'f')) {
				mask[maskIndex].val *= 16;
				mask[maskIndex].val += (uint32_t)(*str - 'a' + 10);
				continue;
			}
		} else {
			if ((*str >= '0') && (*str <= '9')) {
				mask[maskIndex].val *= 10;
				mask[maskIndex].val += (uint32_t)(*str - '0');
				continue;
			}
		}
		if (*str == mask[maskIndex].separator) {
			if ((mask[maskIndex].val < mask[maskIndex].valMin) || (mask[maskIndex].val > mask[maskIndex].valMax))
				return false;
			maskIndex++;
		} else {
			return false;
		}
	}
	/* Validate last segment range (loop exits before separator check for final segment) */
	if (mask[maskIndex].val < mask[maskIndex].valMin || mask[maskIndex].val > mask[maskIndex].valMax)
		return false;
	return true;
}

/**
 * @brief   Validate MAC address (xx:xx:xx:xx:xx:xx) and parse into macAddr.
 */
bool elCliValidateMacaddrValid(const char *str, uint8_t *macAddr)
{
	elCliMask_t mask[6] = {
		{0, 0, 255, ':', true},
		{0, 0, 255, ':', true},
		{0, 0, 255, ':', true},
		{0, 0, 255, ':', true},
		{0, 0, 255, ':', true},
		{0, 0, 255, 0, true}
	};
	if (!elCliValidateByMask(str, mask))
		return false;
	for (int i = 0; i < 6; i++)
		macAddr[i] = (uint8_t)mask[i].val;
	return true;
}

/**
 * @brief   Validate IPv4 address (d.d.d.d) and parse into ipaddr.
 */
bool elCliValidateIpaddrValid(const char *str, uint32_t *ipaddr)
{
	elCliMask_t mask[4] = {
		{0, 0, 255, '.', false},
		{0, 0, 255, '.', false},
		{0, 0, 255, '.', false},
		{0, 0, 255, 0, false}
	};
	if (!elCliValidateByMask(str, mask))
		return false;
	*ipaddr = 0;
	for (int i = 0; i < 4; i++)
		*ipaddr |= mask[i].val << (8U * (3 - i));
	return true;
}

/**
 * @brief   Validate server address (d.d.d.d:port) and parse.
 */
bool elCliValidateSrvAddrValid(const char *str, uint32_t *ipaddr, uint16_t *port)
{
	elCliMask_t mask[5] = {
		{0, 0, 255, '.', false},
		{0, 0, 255, '.', false},
		{0, 0, 255, '.', false},
		{0, 0, 255, ':', false},
		{0, 1024, 65535, 0, false}
	};
	if (!elCliValidateByMask(str, mask))
		return false;
	*ipaddr = 0;
	for (int i = 0; i < 4; i++)
		*ipaddr |= mask[i].val << (8U * (3 - i));
	*port = (uint16_t)mask[4].val;
	return true;
}

/**
 * @brief   Validate integer in range and parse into integer.
 */
bool elCliValidateIntValid(const char *str, int rangeMin, int rangeMax, int *integer)
{
	elCliMask_t mask[1] = {
		{0, (uint32_t)rangeMin, (uint32_t)rangeMax, 0, false}
	};
	if (!elCliValidateByMask(str, mask))
		return false;
	*integer = (int)mask[0].val;
	return true;
}

/**
 * @brief   Validate date (dd/mm/yy or dd/mm/yyyy) and parse.
 */
bool elCliValidateDateValid(const char *str, uint32_t *dateDay, uint32_t *dateMonth, uint32_t *dateYear)
{
	elCliMask_t maskShort[3] = {
		{0, 1, 31, '/', false},
		{0, 1, 12, '/', false},
		{0, 2020, 2080, 0, false}
	};
	elCliMask_t maskFull[3] = {
		{0, 1, 31, '/', false},
		{0, 1, 12, '/', false},
		{0, 20, 80, 0, false}
	};
	if (elCliValidateByMask(str, maskShort)) {
		*dateDay = maskShort[0].val;
		*dateMonth = maskShort[1].val;
		*dateYear = maskShort[2].val;
		return true;
	}
	if (elCliValidateByMask(str, maskFull)) {
		*dateDay = maskFull[0].val;
		*dateMonth = maskFull[1].val;
		*dateYear = maskFull[2].val + 2000U;
		return true;
	}
	return false;
}

/**
 * @brief   Validate time (hh:mm or hh:mm:ss) and parse.
 */
bool elCliValidateTimeValid(const char *str, uint32_t *timeHours, uint32_t *timeMinutes, uint32_t *timeSeconds)
{
	elCliMask_t maskSimple[2] = {
		{0, 0, 23, ':', false},
		{0, 0, 59, 0, false}
	};
	elCliMask_t maskSeconds[3] = {
		{0, 0, 23, ':', false},
		{0, 0, 59, ':', false},
		{0, 0, 59, 0, false}
	};
	if (elCliValidateByMask(str, maskSimple)) {
		*timeHours = maskSimple[0].val;
		*timeMinutes = maskSimple[1].val;
		*timeSeconds = 0;
		return true;
	}
	if (elCliValidateByMask(str, maskSeconds)) {
		*timeHours = maskSeconds[0].val;
		*timeMinutes = maskSeconds[1].val;
		*timeSeconds = maskSeconds[2].val;
		return true;
	}
	return false;
}

#endif /* CONFIG_ELAPI_CLI_EN */
