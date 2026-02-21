/**
 * @file                elrtos.c
 * @brief               ElRTOS-style sys_arch port for lwIP
 *
 * @author              Eldar Mavlyutov
 * @date                2025-11-30
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * Includes arch-specific sys_arch implementation (arch_<target>/arch/cc.c).
 */

#ifdef OS_ELRTOS_EN
#include "arch/cc.c"
#endif