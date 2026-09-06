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

/* Board definition for the Simulator. The pin numbers are placeholders: nothing
 * reads a real GPIO here, they exist so platform-agnostic code compiles. What is
 * real on this platform is SD_DISK_IMAGE, which stands in for the physical card. */

#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

#include <stdbool.h>

/* ---------------------------------------------------------------- SD card -- */

#ifndef SD_SPI_CS
#define SD_SPI_CS 6
#endif
#ifndef SD_SPI_SCLK
#define SD_SPI_SCLK 7
#endif
#ifndef SD_SPI_MOSI
#define SD_SPI_MOSI 8
#endif
#ifndef SD_SPI_MISO
#define SD_SPI_MISO 9
#endif
#ifndef SD_SPI
#define SD_SPI 0
#endif
#ifndef SD_SPI_BAUDRATE
#define SD_SPI_BAUDRATE 25000000
#endif
#ifndef SD_DETECT_PIN
#define SD_DETECT_PIN 10
#endif

/* FAT image standing in for the physical card. Relative, so the binary must be
 * started from the repository root. */
#ifndef SD_DISK_IMAGE
#define SD_DISK_IMAGE "sample/sdcard.img"
#endif

/* -------------------------------------------------------------------- LCD -- */

#ifndef LCD_DC_PIN
#define LCD_DC_PIN 0
#endif
#ifndef LCD_CS_PIN
#define LCD_CS_PIN 1
#endif
#ifndef LCD_CLK_PIN
#define LCD_CLK_PIN 2
#endif
#ifndef LCD_MOSI_PIN
#define LCD_MOSI_PIN 3
#endif
#ifndef LCD_RST_PIN
#define LCD_RST_PIN 4
#endif
#ifndef LCD_BL_PIN
#define LCD_BL_PIN 5
#endif
#ifndef LCD_SPI
#define LCD_SPI 1
#endif

/* -------------------------------------------------------------------- GPS -- */

/* PLACEHOLDER: GPS_UART is only a channel identifier here, matched against the consumer's
 * MOCK_UART_READ table; the pins are never read. */
#ifndef GPS_UART
#define GPS_UART 0
#endif
#ifndef GPS_UART_TX_PIN
#define GPS_UART_TX_PIN 11
#endif
#ifndef GPS_UART_RX_PIN
#define GPS_UART_RX_PIN 12
#endif
#ifndef GPS_UART_BAUDRATE
#define GPS_UART_BAUDRATE 9600
#endif

/* ------------------------------------------------------------ reed switches -- */

/* PLACEHOLDER. Nothing reads a GPIO here. */
#ifndef REED_SWITCH_A_PIN
#define REED_SWITCH_A_PIN 13
#endif
#ifndef REED_SWITCH_B_PIN
#define REED_SWITCH_B_PIN 14
#endif

#endif /* HAL_CONFIG_H */
