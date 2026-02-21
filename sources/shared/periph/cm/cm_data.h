/**
 * @file                cm_data.h
 * @brief               Cryptomodule data path - ARM over AXI Stream DMA.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * Frame crypto over DMA. Runs on data core (home/world).
 * Platform wires elDmaTxInterruptHandler and elDmaRxInterruptHandler via IRQ config.
 * Separate from cm_config (key load on syscpu).
 *
 * RT: Fully IRQ-driven; no busy-wait. RX/TX handlers run in IRQ context.
 *
 * @note Requires CONFIG_ELAPI_HAL_DMA_EN and dma.h.
 */

#pragma once

#include "stdint.h"
#include "dma.h"
#include "elerrcode.h"

/** Max payload size in cmFrame_t.data */
#define CM_MAX_FRAME_LEN  1500

typedef struct cmFrame_t {
	uint8_t iv[16];
	uint16_t framelen;
	uint16_t keyPos;
	uint32_t cryptId;
	uint8_t data[CM_MAX_FRAME_LEN];
} cmFrame_t;

/** RX frame callback. frame/len valid only during callback. */
typedef void (*cmRxFrameHandler_t)(cmFrame_t *frame, unsigned frameLen);

typedef struct cmDataConfig_t {
	uint16_t deviceId;
	unsigned rxIrqId;
	unsigned txIrqId;
	unsigned descriptorCount;
	unsigned maxFrameLen;
} cmDataConfig_t;

#define CM_TX_PENDING_MAX  8

typedef struct cm_t {
	elDma_t *dma;
	cmRxFrameHandler_t rxHandler;
	void *_txPending[CM_TX_PENDING_MAX];
	unsigned _txPendingHead;
	unsigned _txPendingCount;
} cm_t;

/**
 * @brief  Initialize data interface. Performs elDmaInit. Idempotent with same dma.
 * @param  cm     cryptomodule context (zero-initialized)
 * @param  dma    elDma instance (platform-declared, init done here)
 * @param  config DMA configuration
 * @retval el_errcode_e
 */
el_errcode_e cmDataInit(cm_t *cm, elDma_t *dma, const cmDataConfig_t *config);

/**
 * @brief  Send frame (async). Caller may free cmFrame_t after return.
 * @param  cm     cryptomodule context
 * @param  frame  frame to send
 * @retval el_errcode_e
 */
el_errcode_e cmDataSendFrame(cm_t *cm, const cmFrame_t *frame);

/**
 * @brief  Set RX frame callback. Pass NULL to disable.
 * @param  cm      cryptomodule context
 * @param  handler callback (NULL to disable)
 */
void cmDataSetReceiveHandler(cm_t *cm, cmRxFrameHandler_t handler);
