/**
 * @file                cm_config.h
 * @brief               Cryptomodule config/key-load interface (MMIO).
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * Key load: 32 bytes into keyBuffer, keyPos in controlReg, START flag.
 * HW sets KEYLOAD_COMPLETE when done; IRQ recommended.
 *
 * RT: Set cmDriverSetKeyLoadCompleteHandler and wire cmConfigKeyLoadCompleteHandler
 * to IRQ before cmDriverLoadKey. IRQ mode = async, no busy-wait. Polling mode burns CPU.
 */

#pragma once

#include "stdint.h"
#include "elerrcode.h"

typedef struct cmDriver_t cmDriver_t;

typedef struct cmConfig_t {
	uint32_t baseAddress;
	unsigned maxKeysCount;
} cmConfig_t;

/** Key size in bytes (8 * uint32_t) */
#define CM_KEY_SIZE  32

/** Callback when key load completes. arg = user arg, keyPos = loaded slot. */
typedef void (*cmKeyLoadCompleteHandler_t)(void *arg, uint8_t keyPos);

/**
 * @brief  Initialize driver from config. Clears START; waits for HW ready.
 * @param  driver   driver instance (zero-initialized)
 * @param  cfg      config (baseAddress, maxKeysCount)
 * @retval el_errcode_e
 */
el_errcode_e cmDriverInit(cmDriver_t *driver, cmConfig_t *cfg);

/**
 * @brief  Load key. IRQ mode (handler set): async, returns immediately.
 *         Polling mode: blocks on HW; not for RT.
 * @param  driver   driver instance
 * @param  keyPos   key slot (0..maxKeysCount-1)
 * @param  keyData  32 bytes (CM_KEY_SIZE)
 * @retval el_errcode_e
 */
el_errcode_e cmDriverLoadKey(cmDriver_t *driver, uint8_t keyPos, const uint8_t *keyData);

/**
 * @brief  Set key-load-complete callback. Required for RT (avoids polling).
 * @param  driver   driver instance
 * @param  handler  callback (NULL = polling mode)
 * @param  arg      user argument
 */
void cmDriverSetKeyLoadCompleteHandler(cmDriver_t *driver,
	cmKeyLoadCompleteHandler_t handler, void *arg);

/**
 * @brief  Key-load-complete IRQ handler. Wire via platform; arg = cmDriver_t*.
 */
void cmConfigKeyLoadCompleteHandler(void *arg);
