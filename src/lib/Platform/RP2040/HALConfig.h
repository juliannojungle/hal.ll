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

/* Board definition for the RP2040-LCD-1.28. Every macro is #ifndef-guarded so a
 * consuming project can override any of them from its build. */

#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

#include <stdbool.h>

/* ---------------------------------------------------------------- SD card -- */

#ifndef SD_SPI_SCLK
#define SD_SPI_SCLK 18 //2|18
#endif
#ifndef SD_SPI_MOSI
#define SD_SPI_MOSI 19 //3|19
#endif
#ifndef SD_SPI_MISO
#define SD_SPI_MISO 16 //0|16
#endif
#ifndef SD_SPI_CS
#define SD_SPI_CS 17 //1|17
#endif

/* SPI peripheral instance — MUST match the SD_SPI_* pins above.
 * RP2040: GP0-7 / GP16-23 are spi0; GP8-15 / GP26-28 are spi1. */
#ifndef SD_SPI
#define SD_SPI spi0
#endif
#ifndef SD_SPI_BAUDRATE
#define SD_SPI_BAUDRATE (25 * 1000 * 1000)
#endif

/* Card detect switch. LOW = card present, pull-up assumed. Hardware-validated. */
#ifndef SD_DETECT_PIN
#define SD_DETECT_PIN 20 //5|20   // H1 pin 12
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
#define LCD_BL_PIN 25
#endif

/* SPI peripheral instance for the LCD — MUST match the LCD_* pins above. */
#ifndef LCD_SPI
#define LCD_SPI spi1
#endif

/* -------------------------------------------------------------------- GPS -- */

/* PLACEHOLDER. Carried over from pedal.guru's GPS.cpp, which is itself legacy and
 * will change. Nothing on the expansion board is wired to these yet. */
#ifndef GPS_UART
#define GPS_UART uart0
#endif
#ifndef GPS_UART_TX_PIN
#define GPS_UART_TX_PIN 0
#endif
#ifndef GPS_UART_RX_PIN
#define GPS_UART_RX_PIN 1
#endif
#ifndef GPS_UART_BAUDRATE
#define GPS_UART_BAUDRATE 9600
#endif

/* ------------------------------------------------------------ reed switches -- */

/* PLACEHOLDER. The magnet ring's two switches are not wired yet, and the pins have
 * to come from the both-boards overlap analysis before they mean anything. */
#ifndef REED_SWITCH_A_PIN
#define REED_SWITCH_A_PIN 2
#endif
#ifndef REED_SWITCH_B_PIN
#define REED_SWITCH_B_PIN 3
#endif

#endif /* HAL_CONFIG_H */
