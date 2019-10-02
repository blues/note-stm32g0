// Copyright 2018 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

// Turn on/off event handling and the associated sleep mode
#define EVENTS              true
#define EVENT_SLEEP_LED     EVENTS

// Event handling
#if EVENTS

#define EVENT_TIMER         0x00000001
#define EVENT_BUTTON        0x00000002

// Public
void eventPollTimer(void);
bool eventWait(uint32_t events, uint32_t timeoutMs);
uint32_t eventOccurred(void);
void event(uint32_t event);
void eventClear(uint32_t event);

#endif  // EVENTS

// Note that if we use a button, this defines the GPIO where pulling it to ground represents "push"
#ifdef EVENT_BUTTON
#define GPIO_BUTTON_CLOCK_ENABLE    __HAL_RCC_GPIOC_CLK_ENABLE
#define GPIO_BUTTON_PORT            GPIOB
#define GPIO_BUTTON_PIN             GPIO_PIN_4
#define GPIO_BUTTON_IRQ             EXTI4_15_IRQn
#endif

#if EVENT_SLEEP_LED
#define GPIO_LED_ENABLE             __HAL_RCC_GPIOC_CLK_ENABLE
#define GPIO_LED_PORT               GPIOC
#define GPIO_LED_PIN                GPIO_PIN_6
#endif
