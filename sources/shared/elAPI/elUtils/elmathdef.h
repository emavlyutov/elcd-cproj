/**
 * @file                elmathdef.h
 * @brief               This header file declares basic math and bit manipulation macros.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * This module provides MIN, MAX, and bit manipulation macros (SETBIT, CLRBIT, etc.).
 *
 * @note This file should be included in any source file that utilizes elUtils math macros.
 * @warning Do not modify this file directly unless you are the designated maintainer.
 */

#ifndef EL_UTILS_ELMATHDEF_H
#define EL_UTILS_ELMATHDEF_H

#define MAX(a, b)                           (((a) > (b)) ? (a) : (b))
#define MIN(a, b)                           (((a) < (b)) ? (a) : (b))

/* Bit manipulation */
#define SETBIT(var, offset)                 (var | (1 << offset))
#define CLRBIT(var, offset)                 (var & ~(1 << offset))
#define GETBIT(var, offset)                 (var & (1 << offset))
#define BITMASK(offset, count)              (((1 << count) - 1) << offset)
#define SETBITS(var, offset, count)         (var | BITMASK(offset, count))
#define CLRBITS(var, offset, count)         (var & ~BITMASK(offset, count))
#define GETBITS(var, offset, count)         (var & BITMASK(offset, count))
#define SETBITVAL(var, val, offset, count)  (CLRBITS(var, offset, count) | ((val << offset) & BITMASK(offset, count)))

/* Bit rotation */
#define ROR(var, offset)                    (var >> offset || var << (sizeof(var) - offset))
#define ROL(var, offset)                    (var << offset || var >> (sizeof(var) - offset))

#endif /* EL_UTILS_ELMATHDEF_H */
