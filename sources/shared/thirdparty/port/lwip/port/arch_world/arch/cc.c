#ifdef OS_ELRTOS_EN

#include "elrtos.h"
#include "elUtils/elassert.h"

#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"

#include "FreeRTOS.h"
#include "task.h"

#ifndef LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
#define LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX 1
#endif
#ifndef LWIP_FREERTOS_SYS_ARCH_PROTECT_SANITY_CHECK
#define LWIP_FREERTOS_SYS_ARCH_PROTECT_SANITY_CHECK 0
#endif

#define MS_TO_TICKS(ms)  (((ms) + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS)

#if SYS_LIGHTWEIGHT_PROT && LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
static sys_mutex_t sys_arch_protect_mutex;
#endif
#if SYS_LIGHTWEIGHT_PROT && LWIP_FREERTOS_SYS_ARCH_PROTECT_SANITY_CHECK
static sys_prot_t sys_arch_protect_nesting;
#endif

void sys_init(void)
{
#if SYS_LIGHTWEIGHT_PROT && LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
	el_errcode_e ret = elRtosMutexInit(&sys_arch_protect_mutex, true, true);
	LWIP_ASSERT("failed to create sys_arch_protect mutex", ret == EL_SUCCESS);
#endif
}

u32_t sys_now(void)
{
	return (u32_t)xTaskGetTickCount() * portTICK_PERIOD_MS;
}

u32_t sys_jiffies(void)
{
	return (u32_t)xTaskGetTickCount();
}

#if SYS_LIGHTWEIGHT_PROT

sys_prot_t sys_arch_protect(void)
{
#if LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
	el_errcode_e ret = elRtosMutexTake(&sys_arch_protect_mutex, EL_RTOS_NEVER_TIMEOUT, true, false);
	LWIP_ASSERT("sys_arch_protect failed to take the mutex", ret == EL_SUCCESS);
#else
	taskENTER_CRITICAL();
#endif
#if LWIP_FREERTOS_SYS_ARCH_PROTECT_SANITY_CHECK
	{
		sys_prot_t ret = sys_arch_protect_nesting;
		sys_arch_protect_nesting++;
		LWIP_ASSERT("sys_arch_protect overflow", sys_arch_protect_nesting > ret);
		return ret;
	}
#else
	return 1;
#endif
}

void sys_arch_unprotect(sys_prot_t pval)
{
	LWIP_UNUSED_ARG(pval);
#if LWIP_FREERTOS_SYS_ARCH_PROTECT_SANITY_CHECK
	LWIP_ASSERT("unexpected sys_arch_protect_nesting", sys_arch_protect_nesting > 0);
	sys_arch_protect_nesting--;
	LWIP_ASSERT("unexpected sys_arch_protect_nesting", sys_arch_protect_nesting == pval);
#endif

#if LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
	el_errcode_e ret = elRtosMutexGive(&sys_arch_protect_mutex, true, false);
	LWIP_ASSERT("sys_arch_unprotect failed to give the mutex", ret == EL_SUCCESS);
#else
	taskEXIT_CRITICAL();
#endif
}

#endif /* SYS_LIGHTWEIGHT_PROT */

#if !LWIP_COMPAT_MUTEX

err_t sys_mutex_new(sys_mutex_t *mutex)
{
	CHECK_RETURN_ARG_NOT_NULL(mutex, ERR_VAL);
	el_errcode_e ret = elRtosMutexInit((elRtosMutex_t *)mutex, false, true);
	return ret == EL_SUCCESS ? ERR_OK : ERR_MEM;
}

void sys_mutex_lock(sys_mutex_t *mutex)
{
	LWIP_ASSERT("mutex != NULL", mutex != NULL);
	elRtosMutexTake((elRtosMutex_t *)mutex, EL_RTOS_NEVER_TIMEOUT, false, false);
}

void sys_mutex_unlock(sys_mutex_t *mutex)
{
	LWIP_ASSERT("mutex != NULL", mutex != NULL);
	elRtosMutexGive((elRtosMutex_t *)mutex, false, false);
}

void sys_mutex_free(sys_mutex_t *mutex)
{
	CHECK_RETURN_ARG_NOT_NULL(mutex, );
	elRtosMutexDeInit((elRtosMutex_t *)mutex);
	*mutex = NULL;
}

#endif /* !LWIP_COMPAT_MUTEX */

err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
	CHECK_RETURN_ARG_NOT_NULL(sem, ERR_VAL);
	LWIP_ASSERT("initial_count invalid (not 0 or 1)", (count == 0) || (count == 1));
	el_errcode_e ret = elRtosSemaphoreInit((elRtosSemaphore_t *)sem, 1, count, false);
	if (ret != EL_SUCCESS)
		return ERR_MEM;
	return ERR_OK;
}

void sys_sem_signal(sys_sem_t *sem)
{
	LWIP_ASSERT("sem != NULL", sem != NULL);
	elRtosSemaphoreGive((elRtosSemaphore_t *)sem, false, false);
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
	LWIP_ASSERT("sem != NULL", sem != NULL);
	uint32_t ticks = (timeout == 0) ? EL_RTOS_NEVER_TIMEOUT : MS_TO_TICKS(timeout);
	el_errcode_e ret = elRtosSemaphoreTake((elRtosSemaphore_t *)sem, ticks, false, false);
	if (ret != EL_SUCCESS)
		return SYS_ARCH_TIMEOUT;
	return 1;
}

void sys_sem_free(sys_sem_t *sem)
{
	CHECK_RETURN_ARG_NOT_NULL(sem, );
	elRtosSemaphoreDeInit((elRtosSemaphore_t *)sem);
	*sem = NULL;
}

void sys_arch_msleep(u32_t ms)
{
	if (ms > 0) {
		uint32_t ticks = MS_TO_TICKS(ms);
		if (ticks == 0)
			ticks = 1;
		elRtosDelay(ticks);
	}
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	CHECK_RETURN_ARG_NOT_NULL(mbox, ERR_VAL);
	LWIP_ASSERT("size > 0", size > 0);
	el_errcode_e ret = elRtosQueueInit((elRtosQueue_t *)mbox, (uint32_t)size, sizeof(void *));
	return ret == EL_SUCCESS ? ERR_OK : ERR_MEM;
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
	LWIP_ASSERT("mbox != NULL", mbox != NULL);
	el_errcode_e ret = elRtosQueueWrite((elRtosQueue_t *)mbox, &msg, EL_RTOS_NEVER_TIMEOUT, false, false);
	LWIP_ASSERT("mbox post failed", ret == EL_SUCCESS);
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
	CHECK_RETURN_ARG_NOT_NULL(mbox, ERR_VAL);
	el_errcode_e ret = elRtosQueueWrite((elRtosQueue_t *)mbox, &msg, 0, false, false);
	return (ret == EL_SUCCESS) ? ERR_OK : ERR_MEM;
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
	CHECK_RETURN_ARG_NOT_NULL(mbox, ERR_VAL);
	el_errcode_e ret = elRtosQueueWrite((elRtosQueue_t *)mbox, &msg, 0, false, true);
	return (ret == EL_SUCCESS) ? ERR_OK : ERR_MEM;
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
	void *msg_dummy;
	LWIP_ASSERT("mbox != NULL", mbox != NULL);
	if (!msg)
		msg = &msg_dummy;
	uint32_t ticks = (timeout == 0) ? EL_RTOS_NEVER_TIMEOUT : MS_TO_TICKS(timeout);
	el_errcode_e ret = elRtosQueueRead((elRtosQueue_t *)mbox, msg, ticks, false);
	if (ret != EL_SUCCESS) {
		*msg = NULL;
		return SYS_ARCH_TIMEOUT;
	}
	return 1;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	void *msg_dummy;
	LWIP_ASSERT("mbox != NULL", mbox != NULL);
	if (!msg)
		msg = &msg_dummy;
	el_errcode_e ret = elRtosQueueRead((elRtosQueue_t *)mbox, msg, 0, false);
	if (ret != EL_SUCCESS) {
		*msg = NULL;
		return SYS_MBOX_EMPTY;
	}
	return 0;
}

void sys_mbox_free(sys_mbox_t *mbox)
{
	CHECK_RETURN_ARG_NOT_NULL(mbox, );
	elRtosQueueDeInit((elRtosQueue_t *)mbox);
	*mbox = NULL;
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
	sys_thread_t task = NULL;
	el_errcode_e ret = elRtosTaskCreate(&task, (char *)name, (elRtosTaskHandler_t)thread, arg,
			(uint32_t)prio, NULL, (uint32_t)stacksize);
	LWIP_ASSERT("sys_thread_new failed", ret == EL_SUCCESS);
	return task;
}

#endif /* OS_ELRTOS_EN */
