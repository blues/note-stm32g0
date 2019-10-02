// Copyright 2018 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"
#include "event.h"
#include "stm32g0xx_it.h"

// I/O peripheral handles
#if USE_I2C
extern I2C_HandleTypeDef hi2c1;
#endif
#if USE_UART
extern UART_HandleTypeDef huart1;
extern void MY_UART_IRQHandler(UART_HandleTypeDef *huart);
#endif
#ifdef EVENT_TIMER
extern LPTIM_HandleTypeDef hlptim1;
#endif

// Cortex-M4 non-maskable interrupt
void NMI_Handler(void) {
}

// Cortex-M4 hardware fault interrupt
void HardFault_Handler(void) {
    while (1) ;
}

// Memory management fault
void MemManage_Handler(void) {
    while (1) ;
}

// Prefetch or memory access fault
void BusFault_Handler(void) {
    while (1) ;
}

// Undefined instruction or illegal state
void UsageFault_Handler(void) {
    while (1) ;
}

// System service call via SWI instruction
void SVC_Handler(void) {
}

// Debug monitor
void DebugMon_Handler(void) {
}

// Pendable request for system service.
void PendSV_Handler(void) {
}

// System tick timer
void SysTick_Handler(void) {
    HAL_IncTick();
}

// I2C1 event interrupt
#if USE_I2C
void I2C1_IRQHandler(void) {
  if (hi2c1.Instance->ISR & (I2C_FLAG_BERR | I2C_FLAG_ARLO | I2C_FLAG_OVR)) {
    HAL_I2C_ER_IRQHandler(&hi2c1);
  } else {
    HAL_I2C_EV_IRQHandler(&hi2c1);
  }
}
#endif

// USART1 global interrupt
#if USE_UART
void USART1_IRQHandler(void) {
  HAL_UART_IRQHandler(&huart1);
    MY_UART_IRQHandler(&huart1);
}
#endif

// GPIO handler, enhanced from the base ST handler in a way that enables us to distinguish from the multiple
// pins that sharing the same EXTI.
void MY_GPIO_EXTI_IRQHandler(uint16_t GPIO_Pin) {
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_Pin) != RESET) {
        uint16_t GPIO_Line = GPIO_Pin & EXTI->RPR1;
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
        HAL_GPIO_EXTI_Rising_Callback(GPIO_Line);
    }
}

// GPIO interrupts
void EXTI0_1_IRQHandler( void ) {
  MY_GPIO_EXTI_IRQHandler(GPIO_PIN_0|GPIO_PIN_1);
}
void EXTI2_3_IRQHandler( void ) {
  MY_GPIO_EXTI_IRQHandler(GPIO_PIN_2|GPIO_PIN_3);
}
void EXTI4_15_IRQHandler( void ) {
  MY_GPIO_EXTI_IRQHandler(GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);
}

// LPTIM1 global interrupt
#ifdef EVENT_TIMER
void LPTIM1_IRQHandler(void) {
  HAL_LPTIM_IRQHandler(&hlptim1);
}
#endif


