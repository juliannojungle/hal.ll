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

#include <stdbool.h>
#include <pthread.h>

#include "Types.h"
#include "HALConfig.h"

/* Bus handles. The SD_SPI / LCD_SPI macros in HALConfig.h are values of these. */
typedef int HALSPIBus;
typedef int HALUARTBus;

/* Opaque to consumers: only the Mutex* functions below may touch it. */
typedef struct {
    pthread_mutex_t Handle;
} HALMutex;

/* Pico-SDK-compatible constants, so platform-agnostic consumers compile unchanged. */
#define GPIO_IN                 0
#define GPIO_OUT                1
#define GPIO_FUNC_SPI           1
#define GPIO_FUNC_PWM           4
#define GPIO_FUNC_UART          2
#define PWM_CHAN_B              1
#define GPIO_IRQ_EDGE_FALL_MASK 0x04
#define GPIO_IRQ_EDGE_RISE_MASK 0x08

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
