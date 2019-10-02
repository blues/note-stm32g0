// Copyright 2018 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#ifndef __MAIN_H
#define __MAIN_H

// Choose whether to use I2C or SERIAL for the Notecard
#define NOTECARD_USE_I2C 	false
#define NOTECARD_USE_UART   !NOTECARD_USE_I2C

// Include, or remove, support for these peripherals as needed for memory savings
#define USE_I2C             NOTECARD_USE_I2C
#define USE_UART            NOTECARD_USE_UART

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "stm32g0xx_hal.h"

void Error_Handler(void);

#define T_NRST_Pin GPIO_PIN_2
#define T_NRST_GPIO_Port GPIOF
#define T_VCP_TX_Pin GPIO_PIN_2
#define T_VCP_TX_GPIO_Port GPIOA
#define T_VCP_RX_Pin GPIO_PIN_3
#define T_VCP_RX_GPIO_Port GPIOA
#define LD3_Pin GPIO_PIN_6
#define LD3_GPIO_Port GPIOC
#define T_JTMS_Pin GPIO_PIN_13
#define T_JTMS_GPIO_Port GPIOA
#define T_JTCK_Pin GPIO_PIN_14
#define T_JTCK_GPIO_Port GPIOA

long unsigned int millis(void);
void delay(uint32_t ms);
void setup(void);
void loop(void);

bool MY_Debug(void);
void MY_Sleep_DeInit(void);

#include "event.h"
#ifdef EVENT_TIMER
uint32_t MY_TimerMs(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
