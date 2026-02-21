/**
 * @file                trng.c
 * @brief               True Random Number Generator (MMIO).
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * IRQ mode: no busy-wait. Polling mode: spins on control (avoid in RT).
 */

#include "trng.h"
#include "elassert.h"
#include "elmathdef.h"
#include <stddef.h>
#include <string.h>

/** Control register bit flags */
#define TRNG_CLEAR_FLAG    0
#define TRNG_READY_FLAG    1

/** Max bytes per block (31 words) */
#define TRNG_BLOCK_BYTES   (31U * 4U)

#define TRNG_BUFFER_WORDS  31
#define TRNG_CLEAR_WAIT    50U   /* Brief HW settling; typically < 10 cycles */
#define TRNG_POLL_TIMEOUT  10U

typedef struct {
	uint32_t control;
	uint32_t buffer[TRNG_BUFFER_WORDS];
} trngRegs_t;

struct trngDriver_t {
	volatile trngRegs_t *regs;
};

el_errcode_e trngDriverInit(trngDriver_t *driver, trngConfig_t *cfg)
{
	CHECK_RETURN_ARG_NOT_NULL(driver, EL_ERR_EINVAL);
	CHECK_RETURN_ARG_NOT_NULL(cfg, EL_ERR_EINVAL);

	CHECK_RETURN_EXPR(cfg->baseAddress, EL_ERR_EINVAL);
	driver->regs = (volatile trngRegs_t *)cfg->baseAddress;

	driver->regs->control = SETBIT(driver->regs->control, TRNG_CLEAR_FLAG);
	unsigned timeout = TRNG_CLEAR_WAIT;
	while (GETBIT(driver->regs->control, TRNG_READY_FLAG) && timeout--)
		;
	CHECK_RETURN_EXPR(timeout, EL_ERR_TRNG_RESET);
	driver->regs->control = CLRBIT(driver->regs->control, TRNG_CLEAR_FLAG);
	return EL_SUCCESS;
}

el_errcode_e trngDriverGetRand(trngDriver_t *driver, uint8_t *buffer, unsigned count)
{
	CHECK_RETURN_ARG_NOT_NULL(driver, EL_ERR_EINVAL);
	CHECK_RETURN_ARG_NOT_NULL(buffer, EL_ERR_EINVAL);
	CHECK_RETURN_EXPR(count <= TRNG_BLOCK_BYTES, EL_ERR_EINVAL);

	CHECK_RETURN_EXPR(driver->regs, EL_ERR_NOT_INITIALIZED);

	/* CLEAR=1 resets; CLEAR=0 starts. Ensure CLEAR=0 and wait for READY */
	if (GETBIT(driver->regs->control, TRNG_CLEAR_FLAG)) {
		driver->regs->control = CLRBIT(driver->regs->control, TRNG_CLEAR_FLAG);
		unsigned t = TRNG_CLEAR_WAIT;
		while (!GETBIT(driver->regs->control, TRNG_READY_FLAG) && t--)
			;
		CHECK_RETURN_EXPR(t, EL_ERR_TRNG_BUSY);
	}

	CHECK_RETURN_EXPR(GETBIT(driver->regs->control, TRNG_READY_FLAG), EL_ERR_TRNG_BUSY);

	memcpy(buffer, (const void *)driver->regs->buffer, count);

	driver->regs->control = SETBIT(driver->regs->control, TRNG_CLEAR_FLAG);
	unsigned timeout = TRNG_CLEAR_WAIT;
	while (GETBIT(driver->regs->control, TRNG_READY_FLAG) && timeout--)
		;

	return EL_SUCCESS;
}
