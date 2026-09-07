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

#include "HAL.h"
#include "HalMock.h" // IWYU pragma: keep

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

/* ------------------------------------------------------------------ mocks -- */

#define MOCK_COUNT(table) (UINT32)(sizeof(table) / sizeof((table)[0]))

#if defined(MOCK_DIGITAL_READ) || defined(MOCK_UART_READ) || defined(MOCK_SPI_READ)

static int MockIndexOf(const HALMockRead table[], UINT32 count, UINT32 channel) {
    for (UINT32 i = 0; i < count; i++) {
        if (table[i].Channel == channel) return (int)i;
    }

    return -1;
}

/* False for a malformed entry, so the caller keeps its unmocked result instead of
 * dereferencing a missing text or callback. */
static bool MockRead(const HALMockRead *entry, UINT32 *cursor, UINT32 *value) {
    if (entry->Type == HAL_MOCK_CALLBACK) {
        if (entry->Callback == NULL) return false;

        *value = entry->Callback(entry->Channel);
        return true;
    }

    if (entry->Text == NULL || entry->Text[0] == '\0') return false;

    *value = (UINT8)entry->Text[*cursor];
    *cursor = (entry->Text[*cursor + 1] == '\0') ? 0 : *cursor + 1;
    return true;
}

#endif

#ifdef MOCK_DIGITAL_READ
static const HALMockRead digitalReadMocks[] = MOCK_DIGITAL_READ;
static UINT32 digitalReadCursors[MOCK_COUNT(digitalReadMocks)];
#endif

#ifdef MOCK_SPI_READ
static const HALMockRead spiReadMocks[] = MOCK_SPI_READ;
static UINT32 spiReadCursors[MOCK_COUNT(spiReadMocks)];
#endif

#ifdef MOCK_UART_READ
static const HALMockRead uartReadMocks[] = MOCK_UART_READ;
static UINT32 uartReadCursors[MOCK_COUNT(uartReadMocks)];
#endif

/* ------------------------------------------------------------------- GPIO -- */

/* There is no GPIO on a desktop. These exist so platform-agnostic consumers link. */
void GPIOInit(UINT32 pin) { (void)pin; }
void GPIOSetDir(UINT32 pin, UINT32 mode) { (void)pin; (void)mode; }
void GPIOPullUp(UINT32 pin) { (void)pin; }
void GPIOSetFunction(UINT32 pin, UINT32 function) { (void)pin; (void)function; }
void DigitalWrite(UINT32 pin, UINT8 value) { (void)pin; (void)value; }
UINT8 DigitalRead(UINT32 pin) {
    (void)pin;

#ifdef MOCK_DIGITAL_READ
    int index = MockIndexOf(digitalReadMocks, MOCK_COUNT(digitalReadMocks), pin);
    UINT32 value;

    if (index >= 0 && MockRead(&digitalReadMocks[index], &digitalReadCursors[index], &value))
        return (UINT8)value;
#endif

    return 0;
}

void GPIOSetIRQHandler(UINT32 pin, UINT32 events, void (*handler)(UINT32 pin, UINT32 events)) {
    (void)pin;
    (void)events;
    (void)handler;
}

/* -------------------------------------------------------------------- SPI -- */

void SPIInit(HALSPIBus bus, UINT32 speed) { (void)bus; (void)speed; }
void SPISetBaudrate(HALSPIBus bus, UINT32 speed) { (void)bus; (void)speed; }
void SPISetFormat(HALSPIBus bus, UINT8 dataBits, UINT8 cpol, UINT8 cpha) {
    (void)bus; (void)dataBits; (void)cpol; (void)cpha;
}
void SPIWriteByte(HALSPIBus bus, UINT8 value) { (void)bus; (void)value; }
void SPIWriteNByte(HALSPIBus bus, const UINT8 data[], UINT32 len) { (void)bus; (void)data; (void)len; }

#ifdef MOCK_SPI_READ
static bool SPIMockFill(HALSPIBus bus, UINT8 data[], UINT32 len) {
    int index = MockIndexOf(spiReadMocks, MOCK_COUNT(spiReadMocks), (UINT32)bus);

    if (index < 0) return false;

    for (UINT32 i = 0; i < len; i++) {
        UINT32 value;

        if (!MockRead(&spiReadMocks[index], &spiReadCursors[index], &value)) return false;

        data[i] = (UINT8)value;
    }

    return true;
}
#endif

void SPIReadNByte(HALSPIBus bus, UINT8 txFiller, UINT8 data[], UINT32 len) {
    (void)bus;
    (void)txFiller;

#ifdef MOCK_SPI_READ
    if (SPIMockFill(bus, data, len)) return;
#endif

    for (UINT32 i = 0; i < len; i++) data[i] = 0xFF;
}

void SPIWriteReadNByte(HALSPIBus bus, const UINT8 txData[], UINT8 rxData[], UINT32 len) {
    (void)bus;
    (void)txData;

#ifdef MOCK_SPI_READ
    if (SPIMockFill(bus, rxData, len)) return;
#endif

    for (UINT32 i = 0; i < len; i++) rxData[i] = 0xFF;
}

/* -------------------------------------------------------------------- PWM -- */

UINT32 PWMGPIOToSliceNum(UINT32 pin) { (void)pin; return 0; }
void PWMSetWrap(UINT32 slice, UINT32 value) { (void)slice; (void)value; }
void PWMSetChannelLevel(UINT32 slice, UINT32 channel, UINT16 level) {
    (void)slice; (void)channel; (void)level;
}
void PWMSetClockDivider(UINT32 slice, float divider) { (void)slice; (void)divider; }
void PWMSetEnabled(UINT32 slice, bool enable) { (void)slice; (void)enable; }

/* ------------------------------------------------------------------- UART -- */

/* An unmocked bus reports disabled, so consumers detect the absence instead of blocking
 * on reads that never return data. UARTInit is not part of that condition. */
void UARTInit(HALUARTBus bus, UINT32 baudrate, UINT32 txPin, UINT32 rxPin) {
    (void)bus; (void)baudrate; (void)txPin; (void)rxPin;
}
void UARTDeinit(HALUARTBus bus) { (void)bus; }

bool UARTIsEnabled(HALUARTBus bus) {
    (void)bus;

#ifdef MOCK_UART_READ
    return MockIndexOf(uartReadMocks, MOCK_COUNT(uartReadMocks), (UINT32)bus) >= 0;
#else
    return false;
#endif
}

/* A mock never runs out of data. */
bool UARTIsReadable(HALUARTBus bus) {
    return UARTIsEnabled(bus);
}

char UARTGetChar(HALUARTBus bus) {
    (void)bus;

#ifdef MOCK_UART_READ
    int index = MockIndexOf(uartReadMocks, MOCK_COUNT(uartReadMocks), (UINT32)bus);
    UINT32 value;

    if (index >= 0 && MockRead(&uartReadMocks[index], &uartReadCursors[index], &value))
        return (char)value;
#endif

    return '\0';
}
void UARTPuts(HALUARTBus bus, const char *text) { (void)bus; (void)text; }

/* ------------------------------------------------------------------- time -- */

/* nanosleep rather than sleep(), which takes whole seconds and would round any
 * sub-second delay down to zero. */
void Delay(UINT32 milliseconds) {
    struct timespec request;
    request.tv_sec = (time_t)(milliseconds / 1000);
    request.tv_nsec = (long)(milliseconds % 1000) * 1000000L;
    while (nanosleep(&request, &request) == -1 && errno == EINTR) {
        /* resume the remaining time after an interrupted sleep */
    }
}

/* Monotonic, for measuring elapsed time. Wraps every ~49.7 days; subtracting two
 * readings as UINT32 stays correct across the wrap. */
UINT32 TicksMs(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (UINT32)((UINT32)now.tv_sec * 1000u + (UINT32)(now.tv_nsec / 1000000L));
}

void RTCInitialize(void) {
    /* The host clock is already running. */
}

void RTCGetDateTime(DateTime *dateTime) {
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);
    dateTime->Year = (UINT16)(local.tm_year + 1900);
    dateTime->Month = (UINT8)(local.tm_mon + 1);
    dateTime->Day = (UINT8)local.tm_mday;
    dateTime->Hour = (UINT8)local.tm_hour;
    dateTime->Min = (UINT8)local.tm_min;
    dateTime->Sec = (UINT8)local.tm_sec;
}

/* -------------------------------------------------- threads and mutexes -- */

static void *ThreadTrampoline(void *entry) {
    ((void (*)(void))entry)();
    return NULL;
}

void ThreadStart(void (*entry)(void)) {
    pthread_t thread;
    pthread_create(&thread, NULL, ThreadTrampoline, (void *)entry);
    pthread_detach(thread);
}

void MutexInit(HALMutex *mutex) {
    pthread_mutex_init(&mutex->Handle, NULL);
}

void MutexLock(HALMutex *mutex) {
    pthread_mutex_lock(&mutex->Handle);
}

void MutexRelease(HALMutex *mutex) {
    pthread_mutex_unlock(&mutex->Handle);
}

/* ------------------------------------------------------------------ stdio -- */

void STDIOInitAll(void) {
    /* The host stdio is already up. */
}

/* ----------------------------------------------------------------- system -- */

void DeviceRestart(void) {
    exit(EXIT_SUCCESS);
}
