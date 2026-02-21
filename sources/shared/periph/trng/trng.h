/**
 * @file                trng.h
 * @brief               True Random Number Generator interface (MMIO).
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * HW: control reg (CLEAR, READY), buffer. CLEAR=1 resets, CLEAR=0 starts generation.
 * READY=1 when data valid.
 *
 * Uses trngDriverInit + trngDriverGetRand. GetRand blocks until data ready.
 */

#pragma once

#include "stdint.h"
#include "elerrcode.h"
#include "elmathdef.h"

typedef struct trngDriver_t trngDriver_t;

typedef struct trngConfig_t {
	uint32_t baseAddress;
} trngConfig_t;

/**
 * @brief  Initialize TRNG. Clears state; no spin.
 * @param  driver driver instance
 * @param  cfg    config context
 * @retval el_errcode_e
 */
el_errcode_e trngDriverInit(trngDriver_t *driver, trngConfig_t *cfg);

/**
 * @brief  Get random bytes.
 * @param  driver driver instance
 * @param  buffer output buffer
 * @param  count  byte count
 * @retval el_errcode_e
 */
el_errcode_e trngDriverGetRand(trngDriver_t *driver, uint8_t *buffer, unsigned count);
