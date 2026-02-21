/**
 * @file                elerrcode.h
 * @brief               This header file declares error codes for elAPI modules.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * This module defines el_errcode_e enumeration and block-specific error codes.
 *
 * @note This file should be included in any source file that uses ElCyberDev error codes.
 * @warning Do not modify this file directly unless you are the designated maintainer.
 */

#ifndef EL_UTILS_ELERRCODE_H
#define EL_UTILS_ELERRCODE_H

enum {
    EL_ERRBLOCK_COMMON = 0UL,
    EL_ERRBLOCK_RTOS,
    EL_ERRBLOCK_UART,
    EL_ERRBLOCK_IIC,
    EL_ERRBLOCK_SPI,
    EL_ERRBLOCK_GPIO,
    EL_ERRBLOCK_IRQ,
    EL_ERRBLOCK_SDIO,
    EL_ERRBLOCK_EMAC,
    EL_ERRBLOCK_DMA,
    EL_ERRBLOCK_CLI,
    EL_ERRBLOCK_CM,
    EL_ERRBLOCK_TRNG,
    EL_ERRBLOCK_SFP,
};

#define __GENERATE_ERROR_NAME(blname, errname)                                  EL_ERR_##blname##_##errname
/* Block ID in bits 24-31, errval in bits 0-23; use simple arithmetic for integer constant expression */
#define __CALC_ERROR_CODE(blname, errval)                                       \
                    ((EL_ERRBLOCK_##blname) * 16777216UL + (errval))
#define __GENERATE_COMMON_ERRORS(blname)                                        \
                    __GENERATE_ERROR_NAME(blname, INIT),                        \
                    __GENERATE_ERROR_NAME(blname, SELFTEST),                    \
                    __GENERATE_ERROR_NAME(blname, CONFIGURE)
#define __INIT_ERROR_BLOCK(blname)                                              \
                    __GENERATE_ERROR_NAME(blname, GENERAL) = __CALC_ERROR_CODE(blname, 0),\
                    __GENERATE_COMMON_ERRORS(blname)

typedef enum el_errcode_e{
    EL_ERR_NOT_IMPLEMENTED  = (int)0xFFFFFFFFUL,
    EL_SUCCESS              = 0UL,
    EL_ERR_EINVAL,
    EL_ERR_NOMEM,
    EL_ERR_NOT_INITIALIZED,
    __INIT_ERROR_BLOCK(RTOS),
    __GENERATE_ERROR_NAME(RTOS, TASK),
    __GENERATE_ERROR_NAME(RTOS, TASK_STACKSIZE),
    __GENERATE_ERROR_NAME(RTOS, QUEUE),
	__GENERATE_ERROR_NAME(RTOS, QUEUE_SIZE),
    __GENERATE_ERROR_NAME(RTOS, SEMAPHORE),
    __GENERATE_ERROR_NAME(RTOS, MUTEX),
    __GENERATE_ERROR_NAME(RTOS, EVENTS),
    __INIT_ERROR_BLOCK(UART),
    __GENERATE_ERROR_NAME(UART, TRANSMIT),
    __GENERATE_ERROR_NAME(UART, RECV),
    __INIT_ERROR_BLOCK(IIC),
    __GENERATE_ERROR_NAME(IIC, TRANSMIT),
    __INIT_ERROR_BLOCK(SPI),
    __GENERATE_ERROR_NAME(SPI, TRANSMIT),
    __INIT_ERROR_BLOCK(GPIO),
    __INIT_ERROR_BLOCK(IRQ),
    __INIT_ERROR_BLOCK(SDIO),
    __GENERATE_ERROR_NAME(SDIO, READ),
    __GENERATE_ERROR_NAME(SDIO, WRITE),
    __INIT_ERROR_BLOCK(EMAC),
    __GENERATE_ERROR_NAME(EMAC, SEND),
    __GENERATE_ERROR_NAME(EMAC, RECV),
    __INIT_ERROR_BLOCK(DMA),
    __GENERATE_ERROR_NAME(DMA, TRANSMIT),
    __INIT_ERROR_BLOCK(CLI),
    __GENERATE_ERROR_NAME(CLI, QUEUE),
    __GENERATE_ERROR_NAME(CLI, TASK),
    __INIT_ERROR_BLOCK(CM),
    __GENERATE_ERROR_NAME(CM, KEYLOAD),
    __GENERATE_ERROR_NAME(CM, TIMEOUT),
    __GENERATE_ERROR_NAME(CM, BUSY),
    __INIT_ERROR_BLOCK(TRNG),
    __GENERATE_ERROR_NAME(TRNG, TIMEOUT),
    __GENERATE_ERROR_NAME(TRNG, BUSY),
    __GENERATE_ERROR_NAME(TRNG, RESET),
    __INIT_ERROR_BLOCK(SFP),
    __GENERATE_ERROR_NAME(SFP, BUSY),
} el_errcode_e;

#undef __GENERATE_ERROR_NAME
#undef __CALC_ERROR_CODE
#undef __GENERATE_COMMON_ERRORS
#undef __INIT_ERROR_BLOCK

#endif /* EL_UTILS_ELERRCODE_H */
