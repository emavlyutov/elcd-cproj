/**
 * @file                sfp.h
 * @brief               SFP (Small Form-factor Pluggable) transceiver interface.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * SFP EEPROM over I2C (pages 0/1). Request-response: sfpRequestPages starts read,
 * sfpPoll drives it, pagesReadyHandler invoked when done. Interrupts (RX_LOS, TX_FAULT)
 * are wired by platform; platform calls sfpOnEjection(cfg) from its IRQ handler.
 *
 * @todo TX_DISABLE via GPIO when BSP supports software-driven control. Currently TX always enabled.
 *
 * Requires: CONFIG_ELAPI_HAL_IIC_EN.
 */

#pragma once

#include "stdint.h"
#include "elerrcode.h"
#include "iic.h"

#if defined(CONFIG_ELAPI_HAL_GPIO_EN)
#include "gpio.h"
#endif

#define SFP_PAGE_SIZE      256
#define SFP_PAGE0_ADDR     0x50
#define SFP_PAGE1_ADDR     0x51

/** Callback when both pages read. page0/page1 valid during callback only; copy if needed. */
typedef void (*sfpPagesReadyHandler_t)(void *arg, const uint8_t *page0, const uint8_t *page1);

/** Callback when ejection (RX_LOS/TX_FAULT). Platform calls sfpOnEjection from IRQ. */
typedef void (*sfpEjectionHandler_t)(void *arg);

typedef struct sfpConfig_t {
#if defined(CONFIG_ELAPI_HAL_GPIO_EN)
    /** @todo TX_DISABLE: software-driven when BSP ready. Unused while TX always enabled. */
    elGpio_t *gpio;
    unsigned txDisablePort;
    unsigned txDisablePin;
#endif
    elIic_t *iic;                      /**< Platform-provided, inited */
    sfpPagesReadyHandler_t pagesReadyHandler;
    sfpEjectionHandler_t ejectionHandler;
    void *handlerArg;
} sfpConfig_t;

/**
 * @brief  Initialize SFP. No IRQ wiring; platform registers its own handlers.
 * @param  cfg   config context (zero-initialized)
 * @retval el_errcode_e
 */
el_errcode_e sfpInit(sfpConfig_t *cfg);

/**
 * @brief  Start a pages read request. Call sfpPoll() to drive; pagesReadyHandler when done.
 * @param  cfg   config context
 * @retval el_errcode_e  EL_ERR_EINVAL or EL_SUCCESS; EL_ERR_SFP_BUSY if request already in progress
 */
el_errcode_e sfpRequestPages(sfpConfig_t *cfg);

/**
 * @brief  Drive page read. Call periodically after sfpRequestPages until pagesReadyHandler runs.
 * @param  cfg   config context
 * @retval el_errcode_e
 */
el_errcode_e sfpPoll(sfpConfig_t *cfg);

/**
 * @brief  Ejection notification. Call from platform RX_LOS/TX_FAULT IRQ handler; arg = sfpConfig_t*.
 */
void sfpOnEjection(void *arg);
