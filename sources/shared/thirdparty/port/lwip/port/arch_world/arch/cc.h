/*
 * cc.h - lwIP compiler/arch defines for elRTOS port
 */

#ifndef SRC_THIRDPARTY_LWIP_PORT_ARCH_CC_H_
#define SRC_THIRDPARTY_LWIP_PORT_ARCH_CC_H_

#include <stdint.h>
#include <sys/time.h>

#define LWIP_TIMEVAL_PRIVATE 0

#ifndef BYTE_ORDER
#if defined(PROCESSOR_LITTLE_ENDIAN)
#define BYTE_ORDER LITTLE_ENDIAN
#else
#define BYTE_ORDER BIG_ENDIAN
#endif
#endif

#endif /* SRC_THIRDPARTY_LWIP_PORT_ARCH_CC_H_ */
