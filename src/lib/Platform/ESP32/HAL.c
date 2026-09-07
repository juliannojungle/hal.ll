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

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_system.h"
#include <time.h>
#include <sys/time.h>
#include <string.h>

/* ------------------------------------------------------------------- GPIO -- */

void GPIOInit(UINT32 pin) {
    gpio_reset_pin((gpio_num_t)pin);
}

void GPIOSetDir(UINT32 pin, UINT32 mode) {
    gpio_set_direction((gpio_num_t)pin, mode ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT);
}

void GPIOPullUp(UINT32 pin) {
    gpio_pullup_en((gpio_num_t)pin);
}

void GPIOSetFunction(UINT32 pin, UINT32 function) {
    /* Peripheral routing happens in SPIInit / UARTInit / the LEDC config here. */
    (void)pin;
    (void)function;
}

static void (*gpioIRQHandler)(UINT32 pin, UINT32 events) = NULL;

static void IRAM_ATTR GPIOIRQTrampoline(void *arg) {
    if (gpioIRQHandler != NULL) {
        gpioIRQHandler((UINT32)(uintptr_t)arg, GPIO_IRQ_EDGE_FALL_MASK);
    }
}

/* The event mask is accepted for API parity but ESP-IDF selects the edge through
 * gpio_set_intr_type, so only falling/rising are distinguished. */
void GPIOSetIRQHandler(UINT32 pin, UINT32 events, void (*handler)(UINT32 pin, UINT32 events)) {
    gpioIRQHandler = handler;
    gpio_set_intr_type((gpio_num_t)pin,
        (events & GPIO_IRQ_EDGE_RISE_MASK) ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add((gpio_num_t)pin, GPIOIRQTrampoline, (void *)(uintptr_t)pin);
}

void DigitalWrite(UINT32 pin, UINT8 value) {
    gpio_set_level((gpio_num_t)pin, value);
}

UINT8 DigitalRead(UINT32 pin) {
    return (UINT8)gpio_get_level((gpio_num_t)pin);
}

/* -------------------------------------------------------------------- SPI -- */

/* One device handle per host. ESP-IDF talks to devices, not buses, so the handle
 * created in SPIInit has to be looked up again on every transfer. */
#define SPI_MAX_HOSTS 4
static spi_device_handle_t spiHandles[SPI_MAX_HOSTS];

static void SPIAddDevice(HALSPIBus bus, UINT32 speed) {
    spi_device_interface_config_t deviceConfig = {
        .clock_speed_hz = (int)speed,
        .mode = 0,
        .spics_io_num = -1, // chip select is driven by the caller
        .queue_size = 1
    };
    spi_bus_add_device(bus, &deviceConfig, &spiHandles[bus]);
}

void SPIInit(HALSPIBus bus, UINT32 speed) {
    spi_bus_config_t busConfig = {
        .mosi_io_num = (bus == LCD_SPI) ? LCD_MOSI_PIN : SD_SPI_MOSI,
        .miso_io_num = (bus == LCD_SPI) ? -1 : SD_SPI_MISO,
        .sclk_io_num = (bus == LCD_SPI) ? LCD_CLK_PIN : SD_SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 240 * 240 * 2
    };
    spi_bus_initialize(bus, &busConfig, SPI_DMA_CH_AUTO);
    SPIAddDevice(bus, speed);
}

/* ESP-IDF fixes the clock when a device is added, so changing it means dropping the
 * device and adding it again. This has to work: an SD card must be initialized at
 * 400 kHz or below before being switched to full speed. */
void SPISetBaudrate(HALSPIBus bus, UINT32 speed) {
    if (spiHandles[bus] != NULL) {
        spi_bus_remove_device(spiHandles[bus]);
        spiHandles[bus] = NULL;
    }
    SPIAddDevice(bus, speed);
}

void SPISetFormat(HALSPIBus bus, UINT8 dataBits, UINT8 cpol, UINT8 cpha) {
    /* Set through spi_device_interface_config_t.mode in SPIInit. */
    (void)bus; (void)dataBits; (void)cpol; (void)cpha;
}

void SPIWriteByte(HALSPIBus bus, UINT8 value) {
    if (spiHandles[bus] == NULL) return;
    spi_transaction_t transaction = {
        .length = 8,
        .tx_buffer = &value
    };
    spi_device_transmit(spiHandles[bus], &transaction);
}

void SPIWriteNByte(HALSPIBus bus, const UINT8 data[], UINT32 len) {
    if (spiHandles[bus] == NULL) return;
    spi_transaction_t transaction = {
        .length = len * 8,
        .tx_buffer = data
    };
    spi_device_transmit(spiHandles[bus], &transaction);
}

void SPIReadNByte(HALSPIBus bus, UINT8 txFiller, UINT8 data[], UINT32 len) {
    if (spiHandles[bus] == NULL) return;
    memset(data, txFiller, len);
    spi_transaction_t transaction = {
        .length = len * 8,
        .rxlength = len * 8,
        .tx_buffer = data,
        .rx_buffer = data
    };
    spi_device_transmit(spiHandles[bus], &transaction);
}

void SPIWriteReadNByte(HALSPIBus bus, const UINT8 txData[], UINT8 rxData[], UINT32 len) {
    if (spiHandles[bus] == NULL) return;
    spi_transaction_t transaction = {
        .length = len * 8,
        .rxlength = len * 8,
        .tx_buffer = txData,
        .rx_buffer = rxData
    };
    spi_device_transmit(spiHandles[bus], &transaction);
}

/* -------------------------------------------------------------------- PWM -- */

/* LEDC has channels, not slices. The pin is remembered so PWMSetEnabled can build
 * the channel config, which is where ESP-IDF wants the GPIO. */
static UINT32 pwmPin = 0;

UINT32 PWMGPIOToSliceNum(UINT32 pin) {
    pwmPin = pin;
    return 0;
}

void PWMSetWrap(UINT32 slice, UINT32 value) {
    /* Duty resolution is set in the LEDC timer config. */
    (void)slice;
    (void)value;
}

void PWMSetChannelLevel(UINT32 slice, UINT32 channel, UINT16 level) {
    (void)slice;
    (void)channel;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, level);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void PWMSetClockDivider(UINT32 slice, float divider) {
    /* Frequency is set in the LEDC timer config. */
    (void)slice;
    (void)divider;
}

void PWMSetEnabled(UINT32 slice, bool enable) {
    (void)slice;
    if (!enable) return;

    ledc_timer_config_t timerConfig = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timerConfig);

    ledc_channel_config_t channelConfig = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = (int)pwmPin,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channelConfig);
}

/* ------------------------------------------------------------------- UART -- */

#define UART_RX_BUFFER_SIZE 1024

void UARTInit(HALUARTBus bus, UINT32 baudrate, UINT32 txPin, UINT32 rxPin) {
    uart_config_t config = {
        .baud_rate = (int)baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };
    uart_driver_install(bus, UART_RX_BUFFER_SIZE, 0, 0, NULL, 0);
    uart_param_config(bus, &config);
    uart_set_pin(bus, (int)txPin, (int)rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void UARTDeinit(HALUARTBus bus) {
    uart_driver_delete(bus);
}

bool UARTIsEnabled(HALUARTBus bus) {
    return uart_is_driver_installed(bus);
}

bool UARTIsReadable(HALUARTBus bus) {
    size_t available = 0;
    if (uart_get_buffered_data_len(bus, &available) != ESP_OK) return false;
    return available > 0;
}

char UARTGetChar(HALUARTBus bus) {
    uint8_t value = 0;
    uart_read_bytes(bus, &value, 1, portMAX_DELAY);
    return (char)value;
}

void UARTPuts(HALUARTBus bus, const char *text) {
    uart_write_bytes(bus, text, strlen(text));
}

/* ------------------------------------------------------------------- time -- */

/* vTaskDelay yields the CPU. A sub-tick delay would round to 0 ticks and never
 * yield, so it is raised to 1. */
void Delay(UINT32 milliseconds) {
    TickType_t ticks = pdMS_TO_TICKS(milliseconds);
    vTaskDelay(ticks > 0 ? ticks : 1);
}

/* Monotonic, for measuring elapsed time. esp_timer rather than the FreeRTOS tick,
 * whose default resolution is 10 ms. Wraps every ~49.7 days; subtracting two
 * readings as UINT32 stays correct across the wrap. */
UINT32 TicksMs(void) {
    return (UINT32)(esp_timer_get_time() / 1000);
}

/* Seeds the system clock so timestamps are sane without NTP or an external RTC. */
void RTCInitialize(void) {
    struct timeval now = {
        .tv_sec = 1735689600, // 2025-01-01 00:00:00 UTC
        .tv_usec = 0
    };
    settimeofday(&now, NULL);
}

void RTCGetDateTime(DateTime *dateTime) {
    time_t now;
    struct tm local;
    time(&now);
    localtime_r(&now, &local);
    dateTime->Year = (UINT16)(local.tm_year + 1900);
    dateTime->Month = (UINT8)(local.tm_mon + 1);
    dateTime->Day = (UINT8)local.tm_mday;
    dateTime->Hour = (UINT8)local.tm_hour;
    dateTime->Min = (UINT8)local.tm_min;
    dateTime->Sec = (UINT8)local.tm_sec;
}

/* -------------------------------------------------- threads and mutexes -- */

#define THREAD_STACK_SIZE 4096
#define THREAD_PRIORITY   1

static void ThreadTrampoline(void *entry) {
    ((void (*)(void))entry)();
    vTaskDelete(NULL);
}

void ThreadStart(void (*entry)(void)) {
    xTaskCreate(ThreadTrampoline, "hal.ll thread", THREAD_STACK_SIZE,
                (void *)entry, THREAD_PRIORITY, NULL);
}

void MutexInit(HALMutex *mutex) {
    mutex->Handle = xSemaphoreCreateMutex();
}

void MutexLock(HALMutex *mutex) {
    xSemaphoreTake(mutex->Handle, portMAX_DELAY);
}

void MutexRelease(HALMutex *mutex) {
    xSemaphoreGive(mutex->Handle);
}

/* ------------------------------------------------------------------ stdio -- */

void STDIOInitAll(void) {
    /* ESP-IDF brings stdio up through its console component. */
}

/* ----------------------------------------------------------------- system -- */

void DeviceRestart(void) {
    esp_restart();
}
