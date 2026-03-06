/**
 * @file                cli_charfunc.h
 * @brief               elCLI character handling and validation utilities.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * Byte handling, parameter parsing, and validation (MAC, IP, date, time).
 *
 * @note Uses stdint types for portability.
 */

#pragma once

#ifdef CONFIG_ELAPI_CLI_EN

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Regular text */
#define ANSI_COLOR_BLACK             "\x1b[30m"
#define ANSI_COLOR_RED               "\x1b[31m"
#define ANSI_COLOR_GREEN             "\x1b[32m"
#define ANSI_COLOR_YELLOW            "\x1b[33m"
#define ANSI_COLOR_BLUE              "\x1b[34m"
#define ANSI_COLOR_MAGENTA           "\x1b[35m"
#define ANSI_COLOR_CYAN              "\x1b[36m"
#define ANSI_COLOR_WHITE             "\x1b[37m"

/* Regular bold text */
#define ANSI_COLOR_B_BLACK           "\x1b[1;30m"
#define ANSI_COLOR_B_RED             "\x1b[1;31m"
#define ANSI_COLOR_B_GREEN           "\x1b[1;32m"
#define ANSI_COLOR_B_YELLOW          "\x1b[1;33m"
#define ANSI_COLOR_B_BLUE            "\x1b[1;34m"
#define ANSI_COLOR_B_MAGENTA         "\x1b[1;35m"
#define ANSI_COLOR_B_CYAN            "\x1b[1;36m"
#define ANSI_COLOR_B_WHITE           "\x1b[1;37m"

/* Regular underline text */
#define ANSI_COLOR_U_BLACK           "\x1b[4;30m"
#define ANSI_COLOR_U_RED             "\x1b[4;31m"
#define ANSI_COLOR_U_GREEN           "\x1b[4;32m"
#define ANSI_COLOR_U_YELLOW          "\x1b[4;33m"
#define ANSI_COLOR_U_BLUE            "\x1b[4;34m"
#define ANSI_COLOR_U_MAGENTA         "\x1b[4;35m"
#define ANSI_COLOR_U_CYAN            "\x1b[4;36m"
#define ANSI_COLOR_U_WHITE           "\x1b[4;37m"

/* High intensity text */
#define ANSI_COLOR_HI_BLACK          "\x1b[0;90m"
#define ANSI_COLOR_HI_RED            "\x1b[0;91m"
#define ANSI_COLOR_HI_GREEN          "\x1b[0;92m"
#define ANSI_COLOR_HI_YELLOW         "\x1b[0;93m"
#define ANSI_COLOR_HI_BLUE           "\x1b[0;94m"
#define ANSI_COLOR_HI_MAGENTA        "\x1b[0;95m"
#define ANSI_COLOR_HI_CYAN           "\x1b[0;96m"
#define ANSI_COLOR_HI_WHITE          "\x1b[0;97m"

/* Bold high intensity text */
#define ANSI_COLOR_BHI_BLACK         "\x1b[1;90m"
#define ANSI_COLOR_BHI_RED           "\x1b[1;91m"
#define ANSI_COLOR_BHI_GREEN		"\x1b[1;92m"
#define ANSI_COLOR_BHI_YELLOW		"\x1b[1;93m"
#define ANSI_COLOR_BHI_BLUE			"\x1b[1;94m"
#define ANSI_COLOR_BHI_MAGENTA		"\x1b[1;95m"
#define ANSI_COLOR_BHI_CYAN			"\x1b[1;96m"
#define ANSI_COLOR_BHI_WHITE		"\x1b[1;97m"

//Regular background
#define ANSI_COLOR_BG_BLACK			"\x1b[40m"
#define ANSI_COLOR_BG_RED			"\x1b[41m"
#define ANSI_COLOR_BG_GREEN           "\x1b[42m"
#define ANSI_COLOR_BG_YELLOW          "\x1b[43m"
#define ANSI_COLOR_BG_BLUE            "\x1b[44m"
#define ANSI_COLOR_BG_MAGENTA         "\x1b[45m"
#define ANSI_COLOR_BG_CYAN            "\x1b[46m"
#define ANSI_COLOR_BG_WHITE           "\x1b[47m"

/* High intensity background */
#define ANSI_COLOR_BG_HI_BLACK        "\x1b[0;100m"
#define ANSI_COLOR_BG_HI_RED          "\x1b[0;101m"
#define ANSI_COLOR_BG_HI_GREEN        "\x1b[0;102m"
#define ANSI_COLOR_BG_HI_YELLOW       "\x1b[0;103m"
#define ANSI_COLOR_BG_HI_BLUE         "\x1b[0;104m"
#define ANSI_COLOR_BG_HI_MAGENTA      "\x1b[0;105m"
#define ANSI_COLOR_BG_HI_CYAN         "\x1b[0;106m"
#define ANSI_COLOR_BG_HI_WHITE        "\x1b[0;107m"

/* Reset color */
#define ANSI_COLOR_RESET              "\x1b[0m"

typedef enum {
	ELCLI_STRING_COMMAND,
	ELCLI_STRING_UNAME,
	ELCLI_STRING_PWD
} elCliStringType_t;

char elCliRxByteHandle(char ch, char *buffer, int bufferLen, int *byteIndex, elCliStringType_t type);
int8_t elCliGetNumberOfParameters(const char *cmdStr);
const char *elCliGetParameter(const char *cmdStr, unsigned wantParam, size_t *paramLen);

bool elCliValidateMacaddrValid(const char *str, uint8_t *macAddr);
bool elCliValidateIpaddrValid(const char *str, uint32_t *ipaddr);
bool elCliValidateSrvAddrValid(const char *str, uint32_t *ipaddr, uint16_t *port);
bool elCliValidateIntValid(const char *str, int rangeMin, int rangeMax, int *integer);
bool elCliValidateDateValid(const char *str, uint32_t *dateDay, uint32_t *dateMonth, uint32_t *dateYear);
bool elCliValidateTimeValid(const char *str, uint32_t *timeHours, uint32_t *timeMinutes, uint32_t *timeSeconds);

#endif /* CONFIG_ELAPI_CLI_EN */
