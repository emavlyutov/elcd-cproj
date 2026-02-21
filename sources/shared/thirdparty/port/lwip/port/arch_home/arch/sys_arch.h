/*
 * sys_arch.h - lwIP sys_arch types for elRTOS port
 */

#ifndef SRC_THIRDPARTY_LWIP_PORT_ARCH_SYS_ARCH_H_
#define SRC_THIRDPARTY_LWIP_PORT_ARCH_SYS_ARCH_H_

#include "lwip/opt.h"
#include "lwip/arch.h"

#if SYS_LIGHTWEIGHT_PROT
typedef u32_t sys_prot_t;
#endif

#if !LWIP_COMPAT_MUTEX
typedef void * sys_mutex_t;
#define sys_mutex_valid_val(mutex)   ((mutex) != NULL)
#define sys_mutex_valid(mutex)       (((mutex) != NULL) && (*(mutex) != NULL))
#define sys_mutex_set_invalid(mutex) (*(mutex) = NULL)
#endif

typedef void * sys_sem_t;
#define sys_sem_valid_val(sema)   ((sema) != NULL)
#define sys_sem_valid(sema)       (((sema) != NULL) && (*(sema) != NULL))
#define sys_sem_set_invalid(sema) (*(sema) = NULL)

typedef void * sys_mbox_t;
#define sys_mbox_valid_val(mbox)   ((mbox) != NULL)
#define sys_mbox_valid(mbox)       (((mbox) != NULL) && (*(mbox) != NULL))
#define sys_mbox_set_invalid(mbox) (*(mbox) = NULL)

typedef void * sys_thread_t;

void sys_arch_msleep(u32_t ms);
#define sys_msleep(ms) sys_arch_msleep(ms)

#endif /* SRC_THIRDPARTY_LWIP_PORT_ARCH_SYS_ARCH_H_ */
