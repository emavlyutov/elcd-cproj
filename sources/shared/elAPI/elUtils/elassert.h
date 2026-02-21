/**
 * @file                elassert.h
 * @brief               This header file declares check, validation and assertion functionality.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * This module provides macros for argument validation and assertion (CHECK_RETURN_*, CHECK_ASSERT_*).
 *
 * @note This file should be included in any source file that uses elAPI validation macros.
 * @warning Do not modify this file directly unless you are the designated maintainer.
 */

#ifndef EL_UTILS_ELASSERT_H
#define EL_UTILS_ELASSERT_H

#define CHECK_RETURN_ARG_NOT_NULL(arg, ret)                 \
        do {                                                \
            if (!arg)                                       \
                return ret;                                 \
        } while(0)

#define CHECK_RETURN_ARG_NOT_NULL_VOID(arg)                 \
        do {                                                \
            if (!arg)                                       \
                return;                                     \
        } while(0)

#define CHECK_PRINT_ARG_NOT_NULL(arg, fmt, ...)         	\
        do {                                                \
            if (!arg)                                       \
                _PRINT(fmt, _VA_ARGS_)                      \
        } while(0)

#define CHECK_ASSERT_ARG_NOT_NULL(arg)                      \
        do {                                                \
            if (!arg) {                                     \
                while(1);                                   \
            }                                               \
        } while(0)

#define CHECK_RETURN_EXPR(expr, ret)                        \
        do {                                                \
            if (!(expr))                                    \
                return ret;                                 \
        } while(0)

#define CHECK_RETURN_EXPR_VOID(expr)                        \
        do {                                                \
            if (!(expr))                                    \
                return;                                     \
        } while(0)

#define CHECK_PRINT_EXPR(expr)                              \
        do {                                                \
            if (!(expr))                                    \
                _PRINT(fmt, ##args)                         \
        } while(0)

#define CHECK_ASSERT_EXPR(expr)                             \
        do {                                                \
            if (!(expr)) {                                  \
                while(1);                                   \
            }                                               \
        } while(0)

#endif /* EL_UTILS_ELASSERT_H */
