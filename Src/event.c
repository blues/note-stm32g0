// Copyright 2018 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

// This module implements an event handler that enables the STM32 to enter STOP2 during periods when it is waiting
// for interrupts or a timeout.  If this capability is not needed, just set EVENTS to false in the Inc/event.h
// header file.  For this capability to work, the MCU must have one timer configured (generally LPTIM1) that can be
// active while in STOP2 mode.  We configure this timer with an extremely long period (2 seconds) so that we can
// process a coarse-grained timeout while also waiting for up to 31 other interrupt-generated events.
// For demonstration purposes, in event.h we have defined a single GPIO pin that can be used to simulate
// a button press.  In this demonstration, using a jumper to briefly pull that pin to GND will result in a wake
// from STOP2 mode and an early timeout on the delay.

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "stm32g0xx.h"
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_pwr_ex.h"
#include "stm32g0xx_ll_pwr.h"
#include "stm32g0xx_ll_cortex.h"
#include "stm32g0xx_ll_rcc.h"
#include "main.h"
#include "event.h"

#if EVENTS

// Current event state
static uint32_t eventsThatHappened = 0;
static uint32_t eventTimerExpiresMs = 0;

// Forwards
void eventSleep(uint32_t wakeEvents);

// Wait for any of these events to happen, with a high-granularity timeout (granularity defined by LPTIM1)
bool eventWait(uint32_t events, uint32_t timeoutMs) {

    if (timeoutMs != 0) {
#ifdef EVENT_TIMER
        eventClear(EVENT_TIMER);
        events |= EVENT_TIMER;
        eventTimerExpiresMs = MY_TimerMs() + timeoutMs;
#else
        eventTimerExpiresMs = HAL_Ticks() + timeoutMs;
#endif
    }

    // Sleep until any of these events happens
    eventSleep(events);

    // Mask off anything except non-timer events that we're looking for
    events &= eventOccurred();
#ifdef EVENT_TIMER
    events &= ~EVENT_TIMER;
#endif

    // Return TRUE if an event other than timeout occurred
    return (events != 0);

}

// Poll to see if any timer events transpired
#ifdef EVENT_TIMER
void eventPollTimer() {
    if (eventTimerExpiresMs != 0 && MY_TimerMs() >= eventTimerExpiresMs) {
        eventTimerExpiresMs = 0;
        event(EVENT_TIMER);
    }
}
#endif

// Mark that an event has transpired.  Note that this is safe to call from an ISR
void event(uint32_t event) {
    eventsThatHappened |= event;
}

// Get the mask of events that have occurred
uint32_t eventOccurred() {
    return eventsThatHappened;
}

// Mark that we're no longer interested in an event
void eventClear(uint32_t event) {
    eventsThatHappened &= ~event;
}

// Go into a sleep state if it's appropriate to do so, else simply return
void eventSleep(uint32_t wakeEvents) {

    // Exit immediately if the event already occurred
    if ((eventsThatHappened & wakeEvents) != 0)
        return;

    // We can only sleep if we've got a timer that can be active during sleep
#ifndef EVENT_TIMER

    // Do a compute-bound loop waiting for the events or the timer expiration
    while ((eventsThatHappened & wakeEvents) == 0) {
        if (eventTimerExpiresMs != 0 && HAL_Ticks() >= eventTimerExpiresMs)
            break;
        HAL_Delay(1);
    }

#else

    // If any of several conditions hold true, we will wait for events in a loop
    bool highPowerEventWait = false;

    // If we're debugging, don't sleep
    if (MY_Debug())
        highPowerEventWait = true;

    // Loop in here until an event happens
    while ((eventsThatHappened & wakeEvents) == 0) {

        // If we're doing an event wait in a high-power manner, spin
        if (highPowerEventWait) {

            HAL_Delay(1);

        } else {

            // Deinitialize all perpherals
            MY_Sleep_DeInit();

            // Specify that we want flash to be powered down in STOP mode (huge savings)
            LL_PWR_EnableFlashPowerDownInStop();

            // Set Stop mode 2
            LL_PWR_SetPowerMode(LL_PWR_MODE_STOP1);

            // Enabled events and all interrupts, including disabled interrupts,
            // can wake up the processor
            LL_LPM_EnableEventOnPend();

            // Make sure that we don't sleep on return from an interrupt
            LL_LPM_DisableSleepOnExit();

            // Enable content retention
            LL_PWR_EnableSRAMRetention();

            // Set SLEEPDEEP bit of Cortex System Control Register
            LL_LPM_EnableDeepSleep();

            // Give visibility to scheduler because otherwise it's difficult to debug
#if EVENT_SLEEP_LED
            HAL_GPIO_WritePin(GPIO_LED_PORT, GPIO_LED_PIN, GPIO_PIN_RESET);
#endif

            // Wait for interrupt
            __DSB();
            __WFI();
            __ISB();

            // Give visibility to scheduler because otherwise it's difficult to debug
#if EVENT_SLEEP_LED
            HAL_GPIO_WritePin(GPIO_LED_PORT, GPIO_LED_PIN, GPIO_PIN_SET);
#endif

            // Reset SLEEPDEEP bit of Cortex System Control Register
            // so that we only sleep, not deep sleep, on another wfi
            LL_LPM_EnableSleep();

            // Reset clocks (BEFORE GetHCLKFreq() is called)
            SystemCoreClockUpdate();

        }

#endif  // EVENT_TIMER

    }   // loop until an event happens

}

#endif // EVENTS
