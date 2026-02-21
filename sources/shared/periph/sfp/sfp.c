/**
 * @file                sfp.c
 * @brief               SFP transceiver driver (I2C EEPROM, request-response).
 *
 * @author              Eldar Mavlyutov
 * @date                2026-02-14
 * @version             1.0
 * @copyright           (c) 2025 ElCyberDev. All rights reserved.
 *
 * Request-response: sfpRequestPages starts read, sfpPoll drives it, pagesReadyHandler
 * when done. Platform wires IRQ and calls sfpOnEjection from RX_LOS/TX_FAULT handler.
 *
 * @todo TX_DISABLE via GPIO when BSP supports software-driven control. Currently TX always enabled.
 */

#include "sfp.h"
#include "elassert.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef CONFIG_ELAPI_HAL_IIC_EN

typedef enum {
	SFP_STATE_IDLE,
	SFP_STATE_READING_P0,
	SFP_STATE_READING_P1,
} sfpState_e;

static sfpConfig_t *s_cfg;
static sfpState_e s_state;
static uint8_t s_page0[SFP_PAGE_SIZE];
static uint8_t s_page1[SFP_PAGE_SIZE];

static void sfpCancel(sfpConfig_t *cfg);

el_errcode_e sfpInit(sfpConfig_t *cfg)
{
	CHECK_RETURN_ARG_NOT_NULL(cfg, EL_ERR_EINVAL);
	CHECK_RETURN_EXPR(cfg->iic, EL_ERR_EINVAL);

	s_cfg = cfg;
	s_state = SFP_STATE_IDLE;

#if defined(CONFIG_ELAPI_HAL_GPIO_EN)
	/* @todo TX_DISABLE: configure when BSP supports software-driven GPIO. */
	if (cfg->gpio) {
		el_errcode_e err = elGpioConfig(cfg->gpio, cfg->txDisablePort, cfg->txDisablePin,
			GPIO_PULL_NONE, GPIO_MODE_OUTPUT);
		if (err != EL_SUCCESS)
			return err;
		elGpioWritePin(cfg->gpio, cfg->txDisablePort, cfg->txDisablePin, true);
	}
#endif

	return EL_SUCCESS;
}

el_errcode_e sfpRequestPages(sfpConfig_t *cfg)
{
	CHECK_RETURN_ARG_NOT_NULL(cfg, EL_ERR_EINVAL);
	CHECK_RETURN_EXPR(cfg->iic, EL_ERR_EINVAL);
	CHECK_RETURN_EXPR(s_state == SFP_STATE_IDLE, EL_ERR_SFP_BUSY);
	CHECK_RETURN_EXPR(s_cfg == cfg, EL_ERR_EINVAL);

	s_state = SFP_STATE_READING_P0;
	return EL_SUCCESS;
}

el_errcode_e sfpPoll(sfpConfig_t *cfg)
{
	CHECK_RETURN_ARG_NOT_NULL(cfg, EL_ERR_EINVAL);
	CHECK_RETURN_EXPR(cfg->iic, EL_ERR_EINVAL);

	if (s_state == SFP_STATE_READING_P0) {
		unsigned len = SFP_PAGE_SIZE;
		el_errcode_e err = elIicTransmit(cfg->iic, SFP_PAGE0_ADDR, NULL, 0, s_page0, &len);
		if (err == EL_ERR_IIC_TRANSMIT) {
			sfpCancel(cfg);
			return err;
		}
		if (err == EL_SUCCESS)
			s_state = SFP_STATE_READING_P1;
		return EL_SUCCESS;
	}

	if (s_state == SFP_STATE_READING_P1) {
		unsigned len = SFP_PAGE_SIZE;
		el_errcode_e err = elIicTransmit(cfg->iic, SFP_PAGE1_ADDR, NULL, 0, s_page1, &len);
		if (err == EL_ERR_IIC_TRANSMIT) {
			sfpCancel(cfg);
			return err;
		}
		if (err == EL_SUCCESS) {
			s_state = SFP_STATE_IDLE;
#if defined(CONFIG_ELAPI_HAL_GPIO_EN)
			/* @todo TX_DISABLE: drive low (enable TX) when BSP supports software GPIO */
			if (cfg->gpio)
				elGpioWritePin(cfg->gpio, cfg->txDisablePort, cfg->txDisablePin, false);
#endif
			if (cfg->pagesReadyHandler)
				cfg->pagesReadyHandler(cfg->handlerArg, s_page0, s_page1);
		}
		return EL_SUCCESS;
	}

	return EL_SUCCESS;
}

void sfpOnEjection(void *arg)
{
	sfpConfig_t *cfg = (sfpConfig_t *)arg;
	CHECK_RETURN_EXPR_VOID(cfg && s_cfg == cfg);
	sfpCancel(cfg);
}

static void sfpCancel(sfpConfig_t *cfg)
{
	CHECK_RETURN_ARG_NOT_NULL_VOID(cfg);
	s_state = SFP_STATE_IDLE;
#if defined(CONFIG_ELAPI_HAL_GPIO_EN)
	/* @todo TX_DISABLE: drive high (disable TX) when BSP supports software GPIO */
	if (cfg->gpio)
		elGpioWritePin(cfg->gpio, cfg->txDisablePort, cfg->txDisablePin, true);
#endif
	if (cfg->ejectionHandler)
		cfg->ejectionHandler(cfg->handlerArg);
}

#endif /* CONFIG_ELAPI_HAL_IIC_EN */
