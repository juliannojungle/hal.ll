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

/* Board definition for the ESP32-S3-LCD-1.28. Every macro is #ifndef-guarded so a
 * consuming project can override any of them from its build. */

#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

#include <stdbool.h>
#include "driver/spi_master.h"

/* ---------------------------------------------------------------- SD card -- */

#ifndef SD_SPI_SCLK
#define SD_SPI_SCLK 42
#endif
#ifndef SD_SPI_MOSI
#define SD_SPI_MOSI 41
#endif
#ifndef SD_SPI_MISO
#define SD_SPI_MISO 46
#endif
#ifndef SD_SPI_CS
#define SD_SPI_CS 45
#endif

/* SPI host for the SD card. ESP32-S3 routes any GPIO to any SPI host via the
 * GPIO matrix (SPI1_HOST is reserved for flash; SPI2_HOST/SPI3_HOST are free). */
#ifndef SD_SPI
#define SD_SPI SPI2_HOST
#endif
#ifndef SD_SPI_BAUDRATE
#define SD_SPI_BAUDRATE (25 * 1000 * 1000)
#endif

/* Card detect switch. LOW = card present, pull-up assumed. Hardware-validated. */
#ifndef SD_DETECT_PIN
#define SD_DETECT_PIN 39  // H1 pin 12
#endif

/* -------------------------------------------------------------------- LCD -- */

#ifndef LCD_DC_PIN
#define LCD_DC_PIN 8
#endif
#ifndef LCD_CS_PIN
#define LCD_CS_PIN 9
#endif
#ifndef LCD_CLK_PIN
#define LCD_CLK_PIN 10
#endif
#ifndef LCD_MOSI_PIN
#define LCD_MOSI_PIN 11
#endif
#ifndef LCD_RST_PIN
#define LCD_RST_PIN 12
#endif
#ifndef LCD_BL_PIN
#define LCD_BL_PIN 40
#endif

/* Kept on a different host from SD_SPI so LCD and card work on independent buses. */
#ifndef LCD_SPI
#define LCD_SPI SPI3_HOST
#endif

/* -------------------------------------------------------------------- GPS -- */

/* The same physical header positions the RP2040 uses for the GPS, so one expansion board
 * serves both MCUs: TX is H2 pin 2 (RP2040 GP16 = this board's GP13) and RX is H1 pin 11
 * (RP2040 GP13 = GP18). The GPIO matrix routes a UART to any pin here, so unlike the
 * RP2040 there is no TX/RX pin restriction. UART_NUM_0 is left alone because ESP-IDF uses
 * it for the console. Not hardware-validated. */
#ifndef GPS_UART
#define GPS_UART UART_NUM_1
#endif
#ifndef GPS_UART_TX_PIN
#define GPS_UART_TX_PIN 13
#endif
#ifndef GPS_UART_RX_PIN
#define GPS_UART_RX_PIN 18
#endif
#ifndef GPS_UART_BAUDRATE
#define GPS_UART_BAUDRATE 9600
#endif

/* ------------------------------------------------------------ reed switches -- */

/* The same physical header positions the RP2040 uses: H1 pin 13 (RP2040 GP14 = this
 * board's GP17) and H1 pin 15 (RP2040 GP15 = GP16). Reed switch versus hall sensor is
 * still an open choice, which does not change the pins. Not hardware-validated. */
#ifndef REED_SWITCH_A_PIN
#define REED_SWITCH_A_PIN 17
#endif
#ifndef REED_SWITCH_B_PIN
#define REED_SWITCH_B_PIN 16
#endif

#endif /* HAL_CONFIG_H */
