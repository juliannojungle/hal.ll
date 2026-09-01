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
#include "pico/stdio_usb.h"
#include "pico/multicore.h"
#include "hardware/rtc.h"

/* ------------------------------------------------------------------- GPIO -- */

void GPIOInit(UINT32 pin) {
    gpio_init(pin);
}

void GPIOSetDir(UINT32 pin, UINT32 mode) {
    gpio_set_dir(pin, mode);
}

void GPIOPullUp(UINT32 pin) {
    gpio_pull_up(pin);
}

void GPIOSetFunction(UINT32 pin, UINT32 function) {
    gpio_set_function(pin, function);
}

static void (*gpioIRQHandler)(UINT32 pin, UINT32 events) = NULL;

static void GPIOIRQTrampoline(uint pin, uint32_t events) {
    if (gpioIRQHandler != NULL) {
        gpioIRQHandler((UINT32)pin, (UINT32)events);
    }
}

/* The SDK supports one callback for all GPIOs, so the handler is shared and has to
 * dispatch on the pin it receives. */
void GPIOSetIRQHandler(UINT32 pin, UINT32 events, void (*handler)(UINT32 pin, UINT32 events)) {
    gpioIRQHandler = handler;
    gpio_set_irq_enabled_with_callback(pin, events, true, &GPIOIRQTrampoline);
}

void DigitalWrite(UINT32 pin, UINT8 value) {
    gpio_put(pin, value);
}

UINT8 DigitalRead(UINT32 pin) {
    return gpio_get(pin);
}

/* -------------------------------------------------------------------- SPI -- */

void SPIInit(HALSPIBus bus, UINT32 speed) {
    spi_init(bus, speed);
}

void SPISetBaudrate(HALSPIBus bus, UINT32 speed) {
    spi_set_baudrate(bus, speed);
}

void SPISetFormat(HALSPIBus bus, UINT8 dataBits, UINT8 cpol, UINT8 cpha) {
    spi_set_format(bus, dataBits, (spi_cpol_t)cpol, (spi_cpha_t)cpha, SPI_MSB_FIRST);
}

void SPIWriteByte(HALSPIBus bus, UINT8 value) {
    spi_write_blocking(bus, &value, 1);
}

void SPIWriteNByte(HALSPIBus bus, const UINT8 data[], UINT32 len) {
    spi_write_blocking(bus, data, len);
}

void SPIReadNByte(HALSPIBus bus, UINT8 txFiller, UINT8 data[], UINT32 len) {
    spi_read_blocking(bus, txFiller, data, len);
}

void SPIWriteReadNByte(HALSPIBus bus, const UINT8 txData[], UINT8 rxData[], UINT32 len) {
    spi_write_read_blocking(bus, txData, rxData, len);
}

/* -------------------------------------------------------------------- PWM -- */

UINT32 PWMGPIOToSliceNum(UINT32 pin) {
    return pwm_gpio_to_slice_num(pin);
}

void PWMSetWrap(UINT32 slice, UINT32 value) {
    pwm_set_wrap(slice, value);
}

void PWMSetChannelLevel(UINT32 slice, UINT32 channel, UINT16 level) {
    pwm_set_chan_level(slice, channel, level);
}

void PWMSetClockDivider(UINT32 slice, float divider) {
    pwm_set_clkdiv(slice, divider);
}

void PWMSetEnabled(UINT32 slice, bool enable) {
    pwm_set_enabled(slice, enable);
}

/* ------------------------------------------------------------------- UART -- */

void UARTInit(HALUARTBus bus, UINT32 baudrate, UINT32 txPin, UINT32 rxPin) {
    uart_init(bus, baudrate);
    gpio_set_function(txPin, GPIO_FUNC_UART);
    gpio_set_function(rxPin, GPIO_FUNC_UART);
}

void UARTDeinit(HALUARTBus bus) {
    uart_deinit(bus);
}

bool UARTIsEnabled(HALUARTBus bus) {
    return uart_is_enabled(bus);
}

bool UARTIsReadable(HALUARTBus bus) {
    return uart_is_readable(bus);
}

char UARTGetChar(HALUARTBus bus) {
    return uart_getc(bus);
}

void UARTPuts(HALUARTBus bus, const char *text) {
    uart_puts(bus, text);
}

/* ------------------------------------------------------------------- time -- */

void Delay(UINT32 milliseconds) {
    sleep_ms(milliseconds);
}

/* Monotonic, for measuring elapsed time. Wraps every ~49.7 days; subtracting two
 * readings as UINT32 stays correct across the wrap. */
UINT32 TicksMs(void) {
    return (UINT32)to_ms_since_boot(get_absolute_time());
}

/* Seeds the hardware RTC so timestamps are sane without an external time source. */
void RTCInitialize(void) {
    datetime_t dateTime = {
        .year  = 2025,
        .month = 1,
        .day   = 1,
        .dotw  = 3, // Wednesday
        .hour  = 0,
        .min   = 0,
        .sec   = 0
    };
    rtc_init();
    rtc_set_datetime(&dateTime);
}

void RTCGetDateTime(DateTime *dateTime) {
    datetime_t now;
    rtc_get_datetime(&now);
    dateTime->Year = (UINT16)now.year;
    dateTime->Month = (UINT8)now.month;
    dateTime->Day = (UINT8)now.day;
    dateTime->Hour = (UINT8)now.hour;
    dateTime->Min = (UINT8)now.min;
    dateTime->Sec = (UINT8)now.sec;
}

/* -------------------------------------------------- threads and mutexes -- */

/* The RP2040 has two cores and no scheduler, so there is exactly one extra thread
 * of execution available: core 1. A second call would overwrite the first. */
void ThreadStart(void (*entry)(void)) {
    multicore_launch_core1(entry);
}

void MutexInit(HALMutex *mutex) {
    mutex_init(&mutex->Handle);
}

void MutexLock(HALMutex *mutex) {
    mutex_enter_blocking(&mutex->Handle);
}

void MutexRelease(HALMutex *mutex) {
    mutex_exit(&mutex->Handle);
}

/* ------------------------------------------------------------------ stdio -- */

void STDIOInitAll(void) {
    stdio_init_all();
    stdio_set_translate_crlf(&stdio_usb, false);
}
