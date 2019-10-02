// Copyright 2018 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "note.h"

// See Inc/MAIN.H for definitions that select whether to use UART or I2C for the Notecard

// Whether or not debugging determines if we can ever go into STOP2 mode (because doing so messes up the debugger).
// On Cortex-M4 this can be determined dynamically using CoreDebug->DHCSR, but in M0 the CoreDebug registers are
// not available to the CPU.
#define DEBUGGING           true

// I/O device handles
#if USE_I2C
I2C_HandleTypeDef hi2c1;
bool i2c1Initialized = false;
#endif
#if USE_UART
UART_HandleTypeDef huart1;
bool uart1Initialized = false;
#endif

// Low-power timer
#ifdef EVENT_TIMER
LPTIM_HandleTypeDef hlptim1;
uint32_t totalTimerMs = 0;
#endif

// Data used for Notecard I/O functions
#if USE_UART
char serialInterruptBuffer[1];
volatile size_t serialFillIndex = 0;
volatile size_t serialDrainIndex = 0;
uint32_t serialOverruns = 0;
char serialBuffer[512];
#endif

// Forwards
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_I2C1_Init(void);
void MX_USART1_UART_Init(void);
#ifdef EVENT_TIMER
void MX_LPTIM1_Init(void);
#endif
void noteSerialReset(void);
void noteSerialTransmit(uint8_t *text, size_t len, bool flush);
bool noteSerialAvailable(void);
char noteSerialReceive(void);
void noteI2CReset(void);
const char *noteI2CTransmit(uint16_t DevAddress, uint8_t* pBuffer, uint16_t Size);
const char *noteI2CReceive(uint16_t DevAddress, uint8_t* pBuffer, uint16_t Size, uint32_t *avail);

// Main entry point
int main(void) {

    // Initialize peripherals
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
#ifdef EVENT_TIMER
    MX_LPTIM1_Init();
#endif

    // Register callbacks with note-c subsystem that it needs for I/O, memory, timer
    NoteSetFn(malloc, free, delay, millis);

    // Register callbacks for Notecard I/O
#if NOTECARD_USE_I2C
    NoteSetFnI2C(NOTE_I2C_ADDR_DEFAULT, NOTE_I2C_MAX_DEFAULT, noteI2CReset, noteI2CTransmit, noteI2CReceive);
#else
    NoteSetFnSerial(noteSerialReset, noteSerialTransmit, noteSerialAvailable, noteSerialReceive);
#endif

    // Use this method of invoking main app code so that we can re-use familiar Arduino examples
    setup();
    while (true)
        loop();

}

// System clock configuration
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    // Configure the main internal regulator output voltage
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    // Initializes the CPU, AHB and APB busses clocks
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();

    // Initializes the CPU, AHB and APB busses clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
        Error_Handler();

    // Initializes the peripherals clocks
    PeriphClkInit.PeriphClockSelection = 0;
#if USE_UART
    PeriphClkInit.PeriphClockSelection |= RCC_PERIPHCLK_USART1;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
#endif
#if USE_I2C
    PeriphClkInit.PeriphClockSelection |= RCC_PERIPHCLK_I2C1;
    PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
#endif
#ifdef EVENT_TIMER
    PeriphClkInit.PeriphClockSelection |= RCC_PERIPHCLK_LPTIM1;
    PeriphClkInit.Lptim1ClockSelection = RCC_LPTIM1CLKSOURCE_LSI;
#endif
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
        Error_Handler();

}

// I2C1 Initialization
#if USE_I2C
void MX_I2C1_Init(void) {

    // Exit if already done
    if (i2c1Initialized)
        return;
    i2c1Initialized = true;

    // Primary initialization
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x00303D5B;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
        Error_Handler();

    // Configure Analogue filter
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
        Error_Handler();

    // Configure Digital filter
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
        Error_Handler();

}
#endif

// I2C1 De-initialization
#if USE_I2C
void MX_I2C1_DeInit(void) {

    // Exit if already done
    if (!i2c1Initialized)
        return;
    i2c1Initialized = false;
    
    // Deconfigure Analogue filter
    HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_DISABLE);

    // Deinitialize
    HAL_I2C_DeInit(&hi2c1);

}
#endif


// USART1 Initialization
#if USE_UART
void MX_USART1_UART_Init(void) {

    // Exit if already done
    if (uart1Initialized)
        return;
    uart1Initialized = true;

    // Primary initialization
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 9600;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart1) != HAL_OK)
        Error_Handler();

    // FIFO mode configuration
    if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
        Error_Handler();
    if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
        Error_Handler();
    if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
        Error_Handler();

    // Reset our buffer management
    serialFillIndex = serialDrainIndex = serialOverruns = 0;

    // Unused, but included for documentation
    ((void)(serialOverruns));

    // Start the inbound receive
    HAL_UART_Receive_IT(&huart1, (uint8_t *) &serialInterruptBuffer, sizeof(serialInterruptBuffer));

}
#endif

// USART1 IRQ handler
#if USE_UART
void MY_UART_IRQHandler(UART_HandleTypeDef *huart) {

    // See if the transfer is completed
    if (huart->RxXferCount == 0) {
        if (serialFillIndex < sizeof(serialBuffer)) {
            if (serialFillIndex+1 == serialDrainIndex)
                serialOverruns++;
            else
                serialBuffer[serialFillIndex++] = serialInterruptBuffer[0];
        } else {
            if (serialDrainIndex == 1)
                serialOverruns++;
            else {
                serialBuffer[0] = serialInterruptBuffer[0];
                serialFillIndex = 1;
            }
        }
    }

    // Start another receive
    HAL_UART_Receive_IT(&huart1, (uint8_t *) &serialInterruptBuffer, sizeof(serialInterruptBuffer));

}
#endif

// USART1 De-initialization
#if USE_UART
void MX_USART1_UART_DeInit(void) {

    // Exit if already done
    if (!uart1Initialized)
        return;
    uart1Initialized = false;

    // Deinitialize
    HAL_UART_DeInit(&huart1);

}
#endif

// LPTIM1 Initialization
#ifdef EVENT_TIMER
void MX_LPTIM1_Init(void) {

    // Initialize clock
    hlptim1.Instance = LPTIM1;
    hlptim1.Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
    hlptim1.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV1;
    hlptim1.Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
    hlptim1.Init.OutputPolarity = LPTIM_OUTPUTPOLARITY_HIGH;
    hlptim1.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
    hlptim1.Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;
    hlptim1.Init.Input1Source = LPTIM_INPUT1SOURCE_GPIO;
    hlptim1.Init.Input2Source = LPTIM_INPUT2SOURCE_GPIO;
    if (HAL_LPTIM_Init(&hlptim1) != HAL_OK)
        Error_Handler();

    // We want this timer to tick at the slowest possible rate, because
    // any overhead spent in this ISR is simply energy that is burned.
    // Start the timeout function in continuous interrupt mode (stm32l4xx_hal_lptim.c).
    // According to this config, with 32k cloock and compare timeout = 65535,
    // the Timeout period = (compare + 1) / Frequency (32KHz) = 2s
    // Period is the maximum value of the auto-reload counter - can't go higher.
    // Timeout is the value to be placed into the compare register - this is max
#define Period      (uint32_t) 65535
#define Timeout     (uint32_t) 32767
#define LPTIM_MS    2000
    if (HAL_LPTIM_TimeOut_Start_IT(&hlptim1, Period, Timeout) != HAL_OK)
        Error_Handler();

}
#endif

// LPTIM1 deinitialization
#ifdef EVENT_TIMER
void MX_LPTIM1_DeInit(void) {
    HAL_LPTIM_DeInit(&hlptim1);
}
#endif

// Compare match callback in non blocking mode
// NOTE: THIS IS CALLED ONCE PER TICK (EVERY 2 SECONDS AS LPTIM1 IS PROGRAMMED)
#ifdef EVENT_TIMER
void HAL_LPTIM_CompareMatchCallback(LPTIM_HandleTypeDef *hlptim) {

    // Add to the total milliseconds since boot
    totalTimerMs += LPTIM_MS;

    // Poll the event poller to see if any events transpired
    eventPollTimer();

}
#endif

// This returns milliseconds since boot (which may wrap)
#ifdef EVENT_TIMER
uint32_t MY_TimerMs() {
    return totalTimerMs;
}
#endif

// GPIO initialization
void MX_GPIO_Init(void) {

    // GPIO Ports Clock Enable
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // GPIO Ports Clock Enable
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // Configure GPIO pin : T_NRST_Pin
    GPIO_InitStruct.Pin = T_NRST_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(T_NRST_GPIO_Port, &GPIO_InitStruct);

    // Configure GPIO pins : T_VCP_TX_Pin T_VCP_RX_Pin
    GPIO_InitStruct.Pin = T_VCP_TX_Pin|T_VCP_RX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure LED GPIO pin : LD3_Pin
#if EVENT_SLEEP_LED
    GPIO_LED_ENABLE();
    GPIO_InitStruct.Pin = GPIO_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIO_LED_PORT, GPIO_LED_PIN, GPIO_PIN_SET);
#endif

    // Initialize the simulated button, if present
#ifdef EVENT_BUTTON
    GPIO_BUTTON_CLOCK_ENABLE();
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Pin = GPIO_BUTTON_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_BUTTON_PORT, &GPIO_InitStruct);
    HAL_NVIC_SetPriority(GPIO_BUTTON_IRQ, 0, 0);
    HAL_NVIC_EnableIRQ(GPIO_BUTTON_IRQ);
#endif

}

// Called when a GPIO interrupt occurs
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) {

    // Handle the button
#ifdef EVENT_BUTTON
    if ((GPIO_Pin & GPIO_BUTTON_PIN) != 0)
        event(EVENT_BUTTON);
#endif

}

// Primary HAL error handler
void Error_Handler(void) {
}

// Primary Assertion handler
#ifdef  USE_FULL_ASSERT
void assert_failed(char *file, uint32_t line) {
}
#endif

// Computationally-delay the specified number of milliseconds
void delay(uint32_t ms) {
    HAL_Delay(ms);
}

// Get the number of app milliseconds since boot (this will wrap)
long unsigned int millis() {
    return (long unsigned int) HAL_GetTick();
}

// Determine whether or not a debugger is actively connected.  We use
// this to suppress STOP2 mode so that code can be maintained/debugged.
bool MY_Debug() {
	return (DEBUGGING);
}

// Deinitialize everything that might block a sleep from happening
void MY_Sleep_DeInit() {

    // Deinitialize the peripherals
#if USE_I2C
    MX_I2C1_DeInit();
#endif
#if USE_UART
    MX_USART1_UART_DeInit();
#endif

    // Notify the Note subsystem that these will need to be reinitialized
    // on the next call to any of the Note I/O functions
    NoteResetRequired();

}

// Serial port reset procedure, called before any I/O and called again upon I/O error
#if NOTECARD_USE_UART
void noteSerialReset() {
    MX_USART1_UART_DeInit();
    MX_USART1_UART_Init();
}
#endif

// Serial write data function
#if NOTECARD_USE_UART
void noteSerialTransmit(uint8_t *text, size_t len, bool flush) {
    HAL_UART_Transmit(&huart1, text, len, 5000);
}
#endif

// Serial "is anything available" function, which does a read-ahead for data into a serial buffer
#if NOTECARD_USE_UART
bool noteSerialAvailable() {
    return (serialFillIndex != serialDrainIndex);
}
#endif

// Blocking serial read a byte function (generally only called if known to be available)
#if NOTECARD_USE_UART
char noteSerialReceive() {
    char data;
    while (!noteSerialAvailable()) ;
    if (serialDrainIndex < sizeof(serialBuffer))
        data = serialBuffer[serialDrainIndex++];
    else {
        data = serialBuffer[0];
        serialDrainIndex = 1;
    }
    return data;
}
#endif

// I2C reset procedure, called before any I/O and called again upon I/O error
#if NOTECARD_USE_I2C
void noteI2CReset() {
    MX_I2C1_DeInit();
    MX_I2C1_Init();
}
#endif

// Transmits in master mode an amount of data, in blocking mode.     The address
// is the actual address; the caller should have shifted it right so that the
// low bit is NOT the read/write bit. An error message is returned, else NULL if success.
#if NOTECARD_USE_I2C
const char *noteI2CTransmit(uint16_t DevAddress, uint8_t* pBuffer, uint16_t Size) {
    char *errstr = NULL;
    int writelen = sizeof(uint8_t) + Size;
    uint8_t *writebuf = malloc(writelen);
    if (writebuf == NULL) {
        errstr = "i2c: insufficient memory (write)";
    } else {
        writebuf[0] = Size;
        memcpy(&writebuf[1], pBuffer, Size);
        HAL_StatusTypeDef err_code = HAL_I2C_Master_Transmit(&hi2c1, DevAddress<<1, writebuf, writelen, 250);
        free(writebuf);
        if (err_code != HAL_OK) {
            errstr = "i2c: write error";
        }
    }
    return errstr;
}
#endif

// Receives in master mode an amount of data in blocking mode. An error mesage returned, else NULL if success.
#if NOTECARD_USE_I2C
const char *noteI2CReceive(uint16_t DevAddress, uint8_t* pBuffer, uint16_t Size, uint32_t *available) {
    const char *errstr = NULL;
    HAL_StatusTypeDef err_code;

    // Retry transmit errors several times, because it's harmless to do so
    for (int i=0; i<3; i++) {
        uint8_t hdr[2];
        hdr[0] = (uint8_t) 0;
        hdr[1] = (uint8_t) Size;
        HAL_StatusTypeDef err_code = HAL_I2C_Master_Transmit(&hi2c1, DevAddress<<1, hdr, sizeof(hdr), 250);
        if (err_code == HAL_OK) {
            errstr = NULL;
            break;
        }
        errstr = "i2c: write error";
    }

    // Only receive if we successfully began transmission
    if (errstr == NULL) {

        int readlen = Size + (sizeof(uint8_t)*2);
        uint8_t *readbuf = malloc(readlen);
        if (readbuf == NULL) {
            errstr = "i2c: insufficient memory (read)";
        } else {
            err_code = HAL_I2C_Master_Receive(&hi2c1, DevAddress<<1, readbuf, readlen, 10);
            if (err_code != HAL_OK) {
                errstr = "i2c: read error";
            } else {
                uint8_t availbyte = readbuf[0];
                uint8_t goodbyte = readbuf[1];
                if (goodbyte != Size) {
                    errstr = "i2c: incorrect amount of data";
                } else {
                    *available = availbyte;
                    memcpy(pBuffer, &readbuf[2], Size);
                }
            }
            free(readbuf);
        }
    }

    // Done
    return errstr;

}
#endif
