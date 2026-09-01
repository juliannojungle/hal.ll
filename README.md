# hal.ll

> 🦖 **Part of [dot-ll-collection](https://github.com/topics/dot-ll-collection)**

> ⚠️ **This project is under active development. The documentation is growing along the project as it's a work-in-progress.**

This is a lightweight, bare-metal hardware abstraction layer (HAL) library for embedded systems.

It gives an application, or a sibling library, one API for GPIO, SPI, PWM, UART, delays, the real-time
clock, threads and mutexes, plus a single place where the board pinout is defined. The same code
compiles for three platforms:

- **Simulator** — Linux desktop, or Windows via WSL
- **RP2040** — Waveshare [RP2040-LCD-1.28](https://www.waveshare.com/wiki/RP2040-LCD-1.28), via pico-sdk
- **ESP32** — Waveshare [ESP32-S3-LCD-1.28](https://www.waveshare.com/wiki/ESP32-S3-LCD-1.28), via ESP-IDF

`src/lib` is the library. `src/Sample.c` is a usage example, and the root `CMakeLists.txt` exists to
build it.

## Using it from another project

Copy `hal.ll.cmake` into your repository and `include()` it after `project()`:

```cmake
set(HAL_LL_PATH "${CMAKE_CURRENT_LIST_DIR}/path/to/hal.ll")  # optional
include(${CMAKE_CURRENT_LIST_DIR}/hal.ll.cmake)
add_executable(myapp ${SOURCES})
include_directories(${INCLUDE_DIRS})
```

The contract appends to `SOURCES` and `INCLUDE_DIRS` rather than declaring a library target, so several
of these libraries accumulate into one build. If `HAL_LL_PATH` is not set and no checkout is found, it
is downloaded at configure time.

## Building the sample

```bash
# Simulator
cmake -B build -DPLATFORM_NAME=Simulator && cmake --build build
./build/hal.ll

# RP2040 (needs pico-sdk at ~/pico-sdk or PICO_SDK_PATH)
cmake -B build -DPLATFORM_NAME=RP2040 && cmake --build build

# ESP32 (needs ESP-IDF exported in the shell)
source ~/esp-idf/export.sh && idf.py -DPLATFORM_NAME=ESP32 build
```

Pass `-DDEBUGMSGS` to enable the `SHOWDEBUG` traces.

See `AGENTS.md` for the design decisions, the API surface and the known open items.

## License

GNU Affero General Public License v3.0 — see `LICENSE`.
