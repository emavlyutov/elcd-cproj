/**
 * @file                cm_data.c
 * @brief               Cryptomodule data path - ARM over AXI Stream DMA.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * DMA init/send/handlers called here. IRQ wiring done by platform.
 */

#include "cm_data.h"
#include "dma.h"
#include "elassert.h"
#include "elerrcode.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef CONFIG_ELAPI_HAL_DMA_EN

typedef struct {
	void *payload;
	uint16_t len;
} frameBuf_t;

static frameBuf_t *frameBufAlloc(uint16_t payloadLen)
{
	frameBuf_t *frame = (frameBuf_t *)calloc(1, sizeof(frameBuf_t));
	CHECK_RETURN_EXPR(frame, NULL);
	frame->payload = calloc(payloadLen, 1);
	if (!frame->payload) {
		free(frame);
		return NULL;
	}
	frame->len = payloadLen;
	return frame;
}

static void frameBufFree(frameBuf_t *frame)
{
	if (frame) {
		free(frame->payload);
		free(frame);
	}
}

static void cmDmaRxHandler(void *arg, uint8_t *data, unsigned len);
static void cmDmaIntrHandler(void *arg, uint32_t eventMask);

/** @return 0 on success, -1 if queue full */
static int cmTxEnqueue(cm_t *cm, frameBuf_t *frame)
{
	if (cm->_txPendingCount >= CM_TX_PENDING_MAX)
		return -1;
	unsigned i = (cm->_txPendingHead + cm->_txPendingCount) % CM_TX_PENDING_MAX;
	cm->_txPending[i] = frame;
	cm->_txPendingCount++;
	return 0;
}

static frameBuf_t *cmTxDequeue(cm_t *cm)
{
	if (cm->_txPendingCount == 0)
		return NULL;
	frameBuf_t *f = (frameBuf_t *)cm->_txPending[cm->_txPendingHead];
	cm->_txPendingHead = (cm->_txPendingHead + 1) % CM_TX_PENDING_MAX;
	cm->_txPendingCount--;
	return f;
}

el_errcode_e cmDataInit(cm_t *cm, elDma_t *dma, const cmDataConfig_t *config)
{
	CHECK_RETURN_ARG_NOT_NULL(cm, EL_ERR_EINVAL);
	CHECK_RETURN_ARG_NOT_NULL(dma, EL_ERR_EINVAL);
	CHECK_RETURN_ARG_NOT_NULL(config, EL_ERR_EINVAL);
	if (cm->dma == dma)
		return EL_SUCCESS;  /* Idempotent */

	CHECK_RETURN_EXPR(config->descriptorCount > 0, EL_ERR_EINVAL);
	CHECK_RETURN_EXPR(config->maxFrameLen >= (unsigned)offsetof(cmFrame_t, data), EL_ERR_EINVAL);

	cm->dma = dma;
	cm->rxHandler = NULL;
	cm->_txPendingHead = 0;
	cm->_txPendingCount = 0;

	elDmaConfig_t dmaCfg = {
		.deviceId = config->deviceId,
		.rxIrqId = config->rxIrqId,
		.txIrqId = config->txIrqId,
		.descriptorCount = config->descriptorCount,
		.maxFrameLen = config->maxFrameLen,
	};
	if (elDmaInit(dma, &dmaCfg) != EL_SUCCESS)
		return EL_ERR_DMA_INIT;

	elDmaSetRxHandler(dma, cmDmaRxHandler, cm);
	elDmaSetInterruptHandler(dma, cmDmaIntrHandler, cm);
	return EL_SUCCESS;
}

el_errcode_e cmDataSendFrame(cm_t *cm, const cmFrame_t *frame)
{
	CHECK_RETURN_ARG_NOT_NULL(cm, EL_ERR_EINVAL);
	CHECK_RETURN_EXPR(cm->dma, EL_ERR_EINVAL);
	CHECK_RETURN_ARG_NOT_NULL(frame, EL_ERR_EINVAL);
	CHECK_RETURN_EXPR(frame->framelen <= CM_MAX_FRAME_LEN, EL_ERR_EINVAL);

	unsigned len = (unsigned)(offsetof(cmFrame_t, data) + frame->framelen);
	frameBuf_t *fb = frameBufAlloc(len);
	CHECK_RETURN_EXPR(fb, EL_ERR_NOMEM);
	memcpy(fb->payload, frame, len);
	fb->len = len;

	if (cmTxEnqueue(cm, fb) != 0) {
		frameBufFree(fb);
		return EL_ERR_NOMEM;  /* Queue full */
	}
	if (elDmaSend(cm->dma, (const uint8_t *)fb->payload, len) != EL_SUCCESS) {
		(void)cmTxDequeue(cm);
		frameBufFree(fb);
		return EL_ERR_DMA_TRANSMIT;
	}
	return EL_SUCCESS;
}

void cmDataSetReceiveHandler(cm_t *cm, cmRxFrameHandler_t handler)
{
	CHECK_RETURN_ARG_NOT_NULL_VOID(cm);
	cm->rxHandler = handler;
}

static void cmDmaRxHandler(void *arg, uint8_t *data, unsigned len)
{
	cm_t *cm = (cm_t *)arg;
	CHECK_RETURN_EXPR_VOID(cm && cm->rxHandler && data);
	CHECK_RETURN_EXPR_VOID(len >= (unsigned)offsetof(cmFrame_t, data));
	CHECK_RETURN_EXPR_VOID(len <= sizeof(cmFrame_t));
	frameBuf_t *fb = frameBufAlloc(len);
	CHECK_RETURN_EXPR_VOID(fb);
	memcpy(fb->payload, data, len);
	fb->len = len;
	cm->rxHandler((cmFrame_t *)fb->payload, len);
	frameBufFree(fb);
}

static void cmDmaIntrHandler(void *arg, uint32_t eventMask)
{
	cm_t *cm = (cm_t *)arg;
	CHECK_RETURN_ARG_NOT_NULL_VOID(cm);
	if (eventMask & EL_DMA_EVENT_TX_DONE) {
		frameBuf_t *fb = cmTxDequeue(cm);
		if (fb)
			frameBufFree(fb);
	}
	/* EL_DMA_EVENT_ERROR, EL_DMA_EVENT_RX_DONE - no cm action */
}

#endif /* CONFIG_ELAPI_HAL_DMA_EN */
