/**
 * @file                syslog.h
 * @brief               This header file declares logger capability.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * This module provides logging macros (PRINT, LOG_SEND_TO_SRV) and log levels.
 *
 * @note This file should be included in any source file that utilizes elSyslog functionalities.
 * @warning Do not modify this file directly unless you are the designated maintainer.
 */

#ifndef EL_SYSLOG_H
#define EL_SYSLOG_H

#ifndef LOG_LEVEL
#define LOG_LEVEL          DEBUG
#endif /* LOG_LEVEL */

#define PRINT(fmt, args...)                     printf(fmt, ##args)
#define LOG_SEND_TO_SRV(lvl, fmt, ##args)

typedef enum {
    EL_LOG_LVL_DEBUG       = 0UL,
    EL_LOG_LVL_INFO,
    EL_LOG_LVL_WARNING,
    EL_LOG_LVL_ERROR,
    EL_LOG_LVL_CRITICAL,
};

#define syslog(level, fmt, args...)                                         \
                    do {                                                    \
                        if (EL_LOG_LVL##level < EL_LOG_LVL_##LOG_LEVEL)     \
                            break;                                          \
                        PRINT("[%s]"fmt, #level, ##args);                   \
                        LOG_SEND_TO_SRV(level, fmt, ##args)                 \
                    }

#endif /* EL_SYSLOG_H */
