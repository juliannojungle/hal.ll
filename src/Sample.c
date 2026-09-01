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

#include <stdio.h>

static HALMutex mutex;
static UINT32 counter;

static void CounterThread(void) {
    for (int i = 0; i < 5; i++) {
        MutexLock(&mutex);
        counter++;
        MutexRelease(&mutex);
        Delay(100);
    }
}

void app_entry(void) {
    STDIOInitAll();
    RTCInitialize();

    DateTime now;
    RTCGetDateTime(&now);
    printf("hal.ll sample\n");
    printf("clock: %04u-%02u-%02u %02u:%02u:%02u\n",
           now.Year, now.Month, now.Day, now.Hour, now.Min, now.Sec);

    GPIOInit(LCD_BL_PIN);
    GPIOSetDir(LCD_BL_PIN, GPIO_OUT);
    DigitalWrite(LCD_BL_PIN, 1);
    printf("backlight pin %d driven high, reads back as %u\n",
           LCD_BL_PIN, DigitalRead(LCD_BL_PIN));

    /* Which bus, and which pins, is the consumer's choice: the calls below are
     * identical on all three platforms even though the underlying types are not. */
    SPIInit(SD_SPI, SD_SPI_BAUDRATE);
    printf("SD SPI initialized at %d Hz\n", SD_SPI_BAUDRATE);

    UARTInit(GPS_UART, GPS_UART_BAUDRATE, GPS_UART_TX_PIN, GPS_UART_RX_PIN);
    printf("GPS UART enabled: %s\n", UARTIsEnabled(GPS_UART) ? "yes" : "no");

    MutexInit(&mutex);
    counter = 0;
    ThreadStart(CounterThread);

    /* Delay is the only synchronisation here: the point is to show the second thread
     * running and the mutex guarding a shared counter, not to be a correct join. */
    for (int i = 0; i < 5; i++) {
        Delay(150);
        MutexLock(&mutex);
        printf("counter seen from the main thread: %u\n", (unsigned int)counter);
        MutexRelease(&mutex);
    }

    printf("done\n");
}

#ifdef ESP_PLATFORM
void app_main(void) {
    app_entry();
}
#else
int main(void) {
    app_entry();
    return 0;
}
#endif
