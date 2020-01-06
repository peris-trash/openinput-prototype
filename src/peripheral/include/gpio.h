#ifndef __GPIO_H__
#define __GPIO_H__

#include <stm32f10x.h>
#include "systick.h"
#include "nvic.h"
#include "utils.h"
#include "rcc.h"

// GPIO & EXTI helper macros
#define IO_MODE_IN          0x0
#define IO_MODE_OUT_10M     0x1
#define IO_MODE_OUT_2M      0x2
#define IO_MODE_OUT_50M     0x3
#define IO_CFG_IN_ANALOG    0x0
#define IO_CFG_IN_FLOAT     0x4
#define IO_CFG_IN_PULL      0x8
#define IO_CFG_OUT_GP_PP    0x0
#define IO_CFG_OUT_GP_OD    0x4
#define IO_CFG_OUT_AF_PP    0x8
#define IO_CFG_OUT_AF_OD    0xC
#define IO_MODE_CFG(p, m, c)    ((uint32_t)(m | c) << ((p & (uint32_t)7) * 4))
#define IO_EXTI_PORTA       0x0
#define IO_EXTI_PORTB       0x1
#define IO_EXTI_PORTC       0x2
#define IO_EXTI_PORTD       0x3
#define IO_EXTI_PORTE       0x4
#define IO_EXTI_PORTF       0x5
#define IO_EXTI_PORTG       0x6
#define IO_EXTI_CFG(i, p)   ((uint32_t)p << ((i & (uint32_t)3) * 4))

void gpio_init();

#endif