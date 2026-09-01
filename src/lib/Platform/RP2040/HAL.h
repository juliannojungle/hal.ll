/*
    hal.ll is an open-source hardware abstraction layer
    for the dot-ll-collection.
    Copyright (C) 2022, Julianno F. C. Silva (@juliannojungle)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/agpl-3.0.html>.
*/

#ifndef HAL_H
#define HAL_H

/* Pulls in the Pico SDK headers so the SDK constants used by platform-agnostic
 * consumers (GPIO_IN, GPIO_OUT, GPIO_FUNC_SPI, GPIO_FUNC_PWM, GPIO_FUNC_UART,
 * PWM_CHAN_B) are visible. The Simulator and ESP32 ports define them by hand. */
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "pico/mutex.h"

#include <stdbool.h>
#include "Types.h"
#include "HALConfig.h"

/* Bus handles. The SD_SPI / LCD_SPI macros in HALConfig.h are values of these. */
typedef spi_inst_t * HALSPIBus;
typedef uart_inst_t * HALUARTBus;

/* Opaque to consumers: only the Mutex* functions below may touch it. */
typedef struct {
    mutex_t Handle;
} HALMutex;

/* Rising/falling edge selectors for GPIOSetIRQHandler, mirroring the SDK values. */
#define GPIO_IRQ_EDGE_FALL_MASK GPIO_IRQ_EDGE_FALL
#define GPIO_IRQ_EDGE_RISE_MASK GPIO_IRQ_EDGE_RISE

/* ------------------------------------------------------------------- GPIO -- */

void GPIOInit(UINT32 pin);
void GPIOSetDir(UINT32 pin, UINT32 mode);
void GPIOPullUp(UINT32 pin);
void GPIOSetFunction(UINT32 pin, UINT32 function);
void GPIOSetIRQHandler(UINT32 pin, UINT32 events, void (*handler)(UINT32 pin, UINT32 events));
void DigitalWrite(UINT32 pin, UINT8 value);
UINT8 DigitalRead(UINT32 pin);

/* -------------------------------------------------------------------- SPI -- */

void SPIInit(HALSPIBus bus, UINT32 speed);
void SPISetBaudrate(HALSPIBus bus, UINT32 speed);
void SPISetFormat(HALSPIBus bus, UINT8 dataBits, UINT8 cpol, UINT8 cpha);
void SPIWriteByte(HALSPIBus bus, UINT8 value);
void SPIWriteNByte(HALSPIBus bus, const UINT8 data[], UINT32 len);
void SPIReadNByte(HALSPIBus bus, UINT8 txFiller, UINT8 data[], UINT32 len);
void SPIWriteReadNByte(HALSPIBus bus, const UINT8 txData[], UINT8 rxData[], UINT32 len);

/* -------------------------------------------------------------------- PWM -- */

UINT32 PWMGPIOToSliceNum(UINT32 pin);
void PWMSetWrap(UINT32 slice, UINT32 value);
void PWMSetChannelLevel(UINT32 slice, UINT32 channel, UINT16 level);
void PWMSetClockDivider(UINT32 slice, float divider);
void PWMSetEnabled(UINT32 slice, bool enable);

/* ------------------------------------------------------------------- UART -- */

/* Pins are arguments, not macros: no device pinout is assumed by this library. */
void UARTInit(HALUARTBus bus, UINT32 baudrate, UINT32 txPin, UINT32 rxPin);
void UARTDeinit(HALUARTBus bus);
bool UARTIsEnabled(HALUARTBus bus);
bool UARTIsReadable(HALUARTBus bus);
char UARTGetChar(HALUARTBus bus);
void UARTPuts(HALUARTBus bus, const char *text);

/* ------------------------------------------------------------------- time -- */

void Delay(UINT32 milliseconds);
UINT32 TicksMs(void);
void RTCInitialize(void);
void RTCGetDateTime(DateTime *dateTime);

/* -------------------------------------------------- threads and mutexes -- */

void ThreadStart(void (*entry)(void));
void MutexInit(HALMutex *mutex);
void MutexLock(HALMutex *mutex);
void MutexRelease(HALMutex *mutex);

/* ------------------------------------------------------------------ stdio -- */

void STDIOInitAll(void);

#endif /* HAL_H */
