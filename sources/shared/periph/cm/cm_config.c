/**
 * @file                cm_config.c
 * @brief               Cryptomodule config/key-load (MMIO). Runs on config core.
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * IRQ mode: no busy-wait. Polling mode: spins on controlReg (avoid in RT).
 */

#include "cm_config.h"
#include "elassert.h"
#include "elmathdef.h"
#include <stddef.h>

/** Control register bit positions and masks */
#define CM_KEYLOAD_START_BIT       0
#define CM_KEYLOAD_COMPLETE_BIT    1
#define CM_KEYLOAD_KEYPOS_OFFSET   8
#define CM_KEYLOAD_START_MASK      BITMASK(CM_KEYLOAD_START_BIT, 1)
#define CM_KEYLOAD_COMPLETE_MASK   BITMASK(CM_KEYLOAD_COMPLETE_BIT, 1)
#define CM_KEYLOAD_KEYPOS_MASK     BITMASK(CM_KEYLOAD_KEYPOS_OFFSET, 8)

/** Key size in bytes (8 * uint32_t) */
#define CM_KEY_SIZE  32

#define CM_KEYBUFFER_WORDS  8
#define CM_POLL_TIMEOUT     100000U

typedef struct {
	uint32_t keyBuffer[CM_KEYBUFFER_WORDS];
	uint32_t controlReg;
} cmDrvRegs_t;

struct cmDriver_t {
	volatile cmDrvRegs_t *regs;
	unsigned maxKeysCount;
	cmKeyLoadCompleteHandler_t keyLoadHandler;
	void *keyLoadHandlerArg;
};

el_errcode_e cmDriverInit(cmDriver_t *driver, cmConfig_t *cfg)
{
	CHECK_RETURN_ARG_NOT_NULL(driver, EL_ERR_EINVAL);
	CHECK_RETURN_ARG_NOT_NULL(cfg, EL_ERR_EINVAL);

	CHECK_RETURN_EXPR(cfg->baseAddress, EL_ERR_EINVAL);
	driver->regs = (volatile cmDrvRegs_t *)cfg->baseAddress;
	driver->maxKeysCount = cfg->maxKeysCount;
	driver->keyLoadHandler = NULL;
	driver->keyLoadHandlerArg = NULL;

	driver->regs->controlReg = CLRBIT(driver->regs->controlReg, CM_KEYLOAD_START_BIT);
	unsigned timeout = CM_POLL_TIMEOUT;
	while (!GETBIT(driver->regs->controlReg, CM_KEYLOAD_COMPLETE_BIT) && timeout--)
		;
	CHECK_RETURN_EXPR(timeout, EL_ERR_CM_TIMEOUT);
	
	return EL_SUCCESS;
}

el_errcode_e cmDriverLoadKey(cmDriver_t *driver, uint8_t keyPos, const uint8_t *keyData)
{
	CHECK_RETURN_ARG_NOT_NULL(driver, EL_ERR_EINVAL);
	CHECK_RETURN_ARG_NOT_NULL(keyData, EL_ERR_EINVAL);
	CHECK_RETURN_EXPR(keyPos < driver->maxKeysCount, EL_ERR_EINVAL);

	CHECK_RETURN_EXPR(driver->regs, EL_ERR_NOT_INITIALIZED);

	CHECK_RETURN_EXPR(!GETBIT(driver->regs->controlReg, CM_KEYLOAD_START_BIT), EL_ERR_CM_BUSY);

	const uint32_t *src = (const uint32_t *)keyData;
	for (int i = 0; i < CM_KEYBUFFER_WORDS; i++)
		driver->regs->keyBuffer[i] = src[i];

	driver->regs->controlReg = CLRBITS(driver->regs->controlReg, CM_KEYLOAD_KEYPOS_OFFSET, 8);
	driver->regs->controlReg = SETBITVAL(driver->regs->controlReg, keyPos, CM_KEYLOAD_KEYPOS_OFFSET, 8);
	driver->regs->controlReg = SETBIT(driver->regs->controlReg, CM_KEYLOAD_START_BIT);

	if (driver->keyLoadHandler)
		return EL_SUCCESS;  /* IRQ mode: no wait */

	/* Polling mode: spins; avoid in RT */
	unsigned n = CM_POLL_TIMEOUT;
	while (!GETBIT(driver->regs->controlReg, CM_KEYLOAD_COMPLETE_BIT) && n--)
		;
	CHECK_RETURN_EXPR(n != 0, EL_ERR_CM_TIMEOUT);
	driver->regs->controlReg = CLRBITS(driver->regs->controlReg, CM_KEYLOAD_START_BIT, 2);
	return EL_SUCCESS;
}

void cmDriverSetKeyLoadCompleteHandler(cmDriver_t *driver,
	cmKeyLoadCompleteHandler_t handler, void *arg)
{
	CHECK_RETURN_ARG_NOT_NULL_VOID(driver);
	driver->keyLoadHandler = handler;
	driver->keyLoadHandlerArg = arg;
}

void cmConfigKeyLoadCompleteHandler(void *arg)
{
	cmDriver_t *driver = (cmDriver_t *)arg;
	CHECK_RETURN_ARG_NOT_NULL_VOID(driver);
	CHECK_RETURN_EXPR_VOID(driver->regs);
	CHECK_RETURN_EXPR_VOID(GETBIT(driver->regs->controlReg, CM_KEYLOAD_COMPLETE_BIT));
	uint8_t keyPos = (uint8_t)(GETBITS(driver->regs->controlReg, CM_KEYLOAD_KEYPOS_OFFSET, 8) >> CM_KEYLOAD_KEYPOS_OFFSET);
	driver->regs->controlReg = CLRBITS(driver->regs->controlReg, CM_KEYLOAD_START_BIT, 2);
	if (driver->keyLoadHandler)
		driver->keyLoadHandler(driver->keyLoadHandlerArg, keyPos);
}
