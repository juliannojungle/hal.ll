# AGENTS.md

Working notes for AI agents and new contributors on **hal.ll**.

---

## Ground rules

### 1. Everything written to this repository is in English

Code, identifiers, string literals, comments, commit messages, `README.md`, this file. Not a single
variable or comment in another language. The **chat is separate**: Julianno, the lead dev, prefers to
talk in pt-BR, and that says nothing about what gets written to disk.

If you find non-English text anywhere outside the chat, stop and ask the dev.

### 2. The dev takes the decisions, never the agent

Architectural choices, naming, trade-offs, scope, ambiguity in a requirement — **ask the dev**. Do not
decide and proceed, and do not present a decision as if it had already been made.

### 3. Do not make assumptions

Only two things count as true: what can be **validated in the code** and what the **dev states
explicitly**. Everything else is a question. Where this file states something unverified, it says so.

### 4. Comments only when essential

**The code has to speak for itself.** Write a comment only when it carries information the code cannot:
a non-obvious *why*, a hardware or spec quirk, units, a subtle invariant, or a short module header. Do
not restate what the code says, do not narrate step by step, and keep it to a line or two. Long
rationale belongs in this file, not in a header every reader has to scroll past.

### 5. Do not write tests

No unit tests, no test harness, no test build system. Verification is a clean build plus running the
sample on the Simulator or on the device. This is the dev's standing policy across the collection.

### 6. This library is 100% C

**No C++ anywhere in this repository**, and the same holds for every library in the dot-ll-collection.
C++ belongs to pedal.guru, the application, and nowhere else. This is not a style preference: fs.ll,
gui.ll and net.ll are C and consume each other, so a C++ construct in a shared header would break them.

Practical consequences: no classes, no namespaces, no templates, no `nullptr`, no `extern "C"` blocks in
this repository's own headers. Threads and mutexes are therefore a C API (§3), not a class.

### 7. Never invent a pinout in use

Pin numbers for peripherals that are actually wired are hardware facts. Do not derive them, and do not
copy them from a datasheet without confirmation. Some values here are hardware-validated (§4).

Placeholders for peripherals that are **not wired yet** are allowed when the dev says so, and every one
of them carries the word `PLACEHOLDER` in a comment. Today that covers the GPS UART and the reed
switches. Never quietly promote a placeholder to a real assignment.

---

## 1. What this project is

**hal.ll** ("low level hardware abstraction layer") is a small C library that gives an application, or a
sibling library, one API for the MCU peripherals and one place where the board pinout lives.

It exists to fix a concrete problem. fs.ll and gui.ll each shipped their own `HAL.h`, `HAL.c` and
`HALConfig.h`, with the same filenames **and the same include guards**. Only one of the two was ever
textually included, and the build was expected to make gui.ll's win by include-path precedence, plus
deleting fs.ll's `HAL.c` from the source list so `Delay` and `STDIOInitAll` would not collide at link
time. Two invisible, order-dependent mechanisms propping up a duplication. Measured before the split:
the two libraries collided on exactly those two filenames and nothing else.

With the HAL in one place, both mechanisms disappear: there is a single `HAL.h`, a single `HALConfig.h`
and a single `HAL.c`, so include order between the sibling contracts stops mattering.

It is a **library meant to be consumed by other projects**, not an application. `src/lib` is the
library; `src/Sample.c` is a usage example and the root `CMakeLists.txt` exists to build it.

Supported platforms: `Simulator` (Linux desktop, and Windows via WSL), `RP2040` (via pico-sdk) and
`ESP32` (via ESP-IDF).

## 2. Repository layout

```
CMakeLists.txt                      builds src/Sample.c, per platform
hal.ll.cmake                        the build contract; copied into consumer projects
AGENTS.md                           this file
README.md                           user-facing overview
src/Sample.c                        usage example / manual test program
src/lib/Types.h                     UINT8/UINT16/UINT32 and DateTime
src/lib/Helper/Debug.h              SHOWDEBUG traces, enabled by -DDEBUGMSGS
src/lib/Platform/<Platform>/        one folder per platform, same file names in each:
    HAL.h                             the whole API, plus per-platform typedefs
    HAL.c                             the implementation
    HALConfig.h                       board definition (pins, buses, baud rates)
    CMakeLists.txt                    ESP32 only: ESP-IDF component registration
src/Dependency/pico_sdk_import.cmake  stock pico-sdk locator, used by the RP2040 build
```

## 3. The API

Everything is declared in the platform's `HAL.h`. Grouped as it appears there:

| group | functions |
|---|---|
| GPIO | `GPIOInit`, `GPIOSetDir`, `GPIOPullUp`, `GPIOSetFunction`, `GPIOSetIRQHandler`, `DigitalWrite`, `DigitalRead` |
| SPI | `SPIInit`, `SPISetBaudrate`, `SPISetFormat`, `SPIWriteByte`, `SPIWriteNByte`, `SPIReadNByte`, `SPIWriteReadNByte` |
| PWM | `PWMGPIOToSliceNum`, `PWMSetWrap`, `PWMSetChannelLevel`, `PWMSetClockDivider`, `PWMSetEnabled` |
| UART | `UARTInit`, `UARTDeinit`, `UARTIsEnabled`, `UARTIsReadable`, `UARTGetChar`, `UARTPuts` |
| time | `Delay`, `RTCInitialize`, `RTCGetDateTime` |
| threads | `ThreadStart`, `MutexInit`, `MutexLock`, `MutexRelease` |
| stdio | `STDIOInitAll` |

Three decisions inside that surface are worth knowing:

**SPI takes the bus as a parameter.** In gui.ll's original HAL, `SPIWriteByte(value)` was hardwired to
`LCD_SPI` despite the generic name, and fs.ll's `DiskIO.c` bypassed the HAL entirely to talk to `SD_SPI`
through the SDK. A HAL that serves both the panel and the card cannot have a bus-less SPI, so every SPI
call takes a `HALSPIBus`. The `SD_SPI` and `LCD_SPI` macros in `HALConfig.h` are values of that type.

**UART takes the pins as arguments**, not from macros. This is deliberate: assigning the GPS pins is a
hardware decision that has not been taken (§7), and a library must not invent a pinout (ground rule 6).

**Threads and mutexes are a C API.** fs.ll and gui.ll are C, so a C++ class was not an option.
`HALMutex` is a struct whose single field is the platform primitive (`pthread_mutex_t`, pico-sdk
`mutex_t`, FreeRTOS `SemaphoreHandle_t`); only the `Mutex*` functions may touch it.

### Platform differences that matter

| | Simulator | RP2040 | ESP32 |
|---|---|---|---|
| `HALSPIBus` | `int` | `spi_inst_t *` | `spi_host_device_t` |
| `HALUARTBus` | `int` | `uart_inst_t *` | `uart_port_t` |
| GPIO/SPI/PWM | no-op stubs | pico-sdk | ESP-IDF; PWM through LEDC |
| `UARTIsEnabled` | always `false` | `uart_is_enabled` | `uart_is_driver_installed` |
| `Delay` | `nanosleep` | `sleep_ms` | `vTaskDelay`, floored at 1 tick |
| `RTCInitialize` | no-op, host clock | hardware RTC seeded to 2025-01-01 | `settimeofday` to 2025-01-01 UTC |
| `ThreadStart` | `pthread_create` + detach | `multicore_launch_core1` | `xTaskCreate` |
| entry point | `main()` | `main()` | `app_main()` (`ESP_PLATFORM` defined) |

`RTCInitialize` seeds a fixed date on the two hardware platforms so timestamps are sane without an
external time source. That is inherited behaviour from fs.ll, not a new choice.

**`ThreadStart` on the RP2040 can only be called once.** It launches core 1, and the RP2040 has no
scheduler, so there is exactly one extra thread of execution available; a second call would overwrite
the first. The other two platforms have no such limit, so code written against them can look fine and
fail here.

**The ESP32 PWM mapping is lossy.** LEDC has channels rather than slices, so `PWMGPIOToSliceNum` just
remembers the pin and returns 0, `PWMSetWrap` and `PWMSetClockDivider` are no-ops (resolution and
frequency are fixed in the LEDC timer config inside `PWMSetEnabled`), and everything drives
`LEDC_CHANNEL_0`. It is enough for one backlight and nothing more. Inherited from gui.ll.

**`SPISetBaudrate` and `SPISetFormat` are no-ops on ESP32.** ESP-IDF fixes clock and mode when the
device is added to the bus; changing them means removing and re-adding the device, which would
invalidate the handle the transfer functions hold.

## 4. The board definition

`HALConfig.h` holds the pinout, one file per platform. Every macro is `#ifndef`-guarded, so a consuming
project can override any of them from its build — that is how the single board definition ends up in the
consumer without this library having to know about it.

Two facts carried over from fs.ll and gui.ll, both to be preserved:

- **The card-detect polarity is hardware-validated**: `SD_DETECT_PIN` reads LOW when a card is present,
  with a pull-up assumed. Do not "correct" it.
- **`SD_SPI` / `LCD_SPI` must match their pin group.** On the RP2040 that is a hard constraint of the
  silicon: GP0-7 and GP16-23 belong to `spi0`, GP8-15 and GP26-28 to `spi1`. On the ESP32-S3 the GPIO
  matrix routes any pin to any host, so the two are kept on separate hosts only so the panel and the
  card do not share a bus.

On the Simulator the pin numbers are placeholders — nothing reads a GPIO. What is real there is
`SD_DISK_IMAGE`, the FAT image that stands in for the physical card. It is a **relative** path, so a
consumer's binary has to be started from its repository root.

## 5. Build system

There is no `add_library`. The contract publishes two list variables that the consumer feeds into its
own target: `SOURCES` gets the platform's `HAL.c`, `INCLUDE_DIRS` gets `src/lib`, `src/lib/Helper` and
the platform folder. Both are **appended to**, never overwritten, so several libraries following this
architecture accumulate into one build.

### `hal.ll.cmake`

| variable | role |
|---|---|
| `HAL_LL_PATH` | in/out. Root of the hal.ll checkout. Accepted as a normal variable or an environment variable; relative paths resolve against `CMAKE_SOURCE_DIR`. Defaults to a `hal.ll` folder next to the copied file. Ends up in the cache. |
| `PLATFORM_NAME` | in. `Simulator` (default), `RP2040` or `ESP32`. |
| `HAL_LL_PLATFORM_DIR` | out. The resolved platform folder. |

Resolution is two steps: default the path, then check the sentinel file `src/lib/Types.h`. If the
sentinel is missing, the directory is populated with a shallow `git clone` at configure time. The
download is deliberately **not** `FetchContent` — see fs.ll's `AGENTS.md` for the full rationale.

**Two rules this file must obey**, both consequences of ESP-IDF's build model:

1. **Variables and messages only.** ESP-IDF evaluates the component `CMakeLists.txt` that includes this
   file in **script mode** (`cmake -P`, through `component_get_requirements.cmake`) to collect `REQUIRES`
   before the real configure. Directory-scoped commands such as `add_compile_definitions` do not exist
   in that mode and abort the ESP32 configure.
2. **Never clobber a `PLATFORM_NAME` the caller already set.** In script mode there is no cache, so an
   unguarded `set(... CACHE ...)` is *not* skipped and would silently reset the platform.

### Root `CMakeLists.txt`

Sets `HAL_LL_PATH` to `CMAKE_SOURCE_DIR` up front, because for an in-tree build this repository *is* the
hal.ll root and nothing should be downloaded. Then it branches on `PLATFORM_NAME`, and closes the chain
with an `else()` that raises `FATAL_ERROR` — without it an unknown platform matches no branch and cmake
reports success while writing a build system with no target.

`-Wall -Wextra` is enabled on the sample and the library on Simulator and RP2040; ESP-IDF applies its
own `-Wall -Werror=all` on ESP32.

## 6. Building and running

`CMAKE_EXPORT_COMPILE_COMMANDS` is on and `.clangd` reads `build/compile_commands.json`, so **build into
`build/`** for working code intelligence. `.clangd` also strips the ARM flags so a desktop clangd can
parse the embedded builds, and defines `DEBUGMSGS`.

```bash
# Simulator
cmake -B build -DPLATFORM_NAME=Simulator && cmake --build build
./build/hal.ll

# RP2040 (needs pico-sdk at ~/pico-sdk or PICO_SDK_PATH)
cmake -B build -DPLATFORM_NAME=RP2040 && cmake --build build

# ESP32 (needs ESP-IDF exported in the shell)
source ~/esp-idf/export.sh && idf.py -DPLATFORM_NAME=ESP32 build
```

`.vscode/tasks.json` has the equivalent build tasks. Running and debugging the Simulator sample is
`.vscode/launch.json` ("Debug Simulator"), which builds first through `preLaunchTask` and points
`miDebuggerPath` at `Toolchain/Simulator/gdb-wrapper.sh`. That wrapper exists because GDB 15 reads
`DEBUGINFOD_URLS` from the environment and blocks for about ten seconds per loaded shared object trying
to fetch debug info; `set debuginfod enabled off` is processed after the initial library loads, so only
unsetting the variable works. Same arrangement as gui.ll.

## 7. Current status

**All three platforms configure, compile and link clean, with zero warnings** in this repository's own
code. Verified this session:

| platform | result |
|---|---|
| Simulator | builds, **and the sample runs correctly** — clock read, GPIO stub, SPI init, and the second thread incrementing a mutex-guarded counter |
| RP2040 | builds and links, produces `hal.ll.uf2` |
| ESP32 | builds and links, produces `hal.ll.bin` (~218 KB) |

**Neither firmware has been flashed or run.** Everything below the Simulator line is established by
compiling and by reading, nothing more. In particular the UART, `GPIOSetIRQHandler` and the ESP32 SPI
read paths are **new code written for this library** and have never executed anywhere.

Nothing consumes hal.ll yet. It was built first, on purpose: fs.ll, gui.ll and net.ll migrate onto it in
later waves, and pedal.guru will consume it directly for sensor access.

## 8. Open items

Each needs a decision from the dev. None is scheduled.

- **The GPS UART and reed-switch pins are placeholders**, marked as such in every `HALConfig.h`. Real
  assignment needs the both-boards overlap analysis — only header pins equivalent on the RP2040 and the
  ESP32-S3 may be used — which lives in gui.ll's `AGENTS.md`. The RP2040 GPS values were carried over
  from pedal.guru's `GPS.cpp`, itself legacy code that will change. The reed switches will also need
  `GPIOSetIRQHandler`, which is written but has never run.
- **Symbol names are unprefixed globals**, and that is a known trade-off rather than an oversight.
  `Delay`, `DigitalWrite`, `MutexLock` and the rest are plain C symbols in the global namespace. They
  were kept exactly as gui.ll had them so migrating fs.ll and gui.ll stays mechanical and
  compiler-verified. If a collision ever shows up in practice, the fix is a prefix pass — but note the
  names have to stay **generic**: this library never knows what a given SPI bus or UART is being used
  for, only the consumer does, so semantic names like `LCDWriteByte` would be wrong here.
- **`Toolchain/` here is a stub.** It holds only `gdb-wrapper.sh`, copied from gui.ll. gui.ll's
  `Toolchain/` is the complete one — environment setup, flashing, disk-image building — and it is reused
  across the whole collection and pedal.guru, so the dev expects it to become its own project later. This
  copy is temporary and should not grow.
- **The licence header wording in this repository was written by the agent** and has not been approved.
  fs.ll and gui.ll carry pedal.guru's wording ("Pedal.guru is an open-source software for cycle
  computers…"), which does not fit a standalone HAL, so the files here say "hal.ll is an open-source
  hardware abstraction layer for the dot-ll-collection". Confirm or replace it.
- **`GPIOSetIRQHandler` supports one handler for all pins.** That is the pico-sdk's own model
  (`gpio_set_irq_enabled_with_callback` is global), so the handler receives the pin and must dispatch on
  it. With two reed switches plus card detect there will be three sources; a per-pin registry would be
  friendlier and is worth revisiting when they are wired.
- **`SPIInit` on ESP32 infers the pins from the bus** by comparing against `LCD_SPI`, because ESP-IDF
  needs the pin numbers at bus-initialization time while the API only receives the bus. It works for the
  two buses this board has and would need rethinking for a third.

## 9. Preserved: the SDL-aware `Delay`

The dev asked for this to be kept. gui.ll's Simulator `Delay` was not a plain sleep — it polled the LCD
layer so a delay in progress would abort when the window was closed:

```c
void Delay(UINT32 milliseconds) {
    UINT32 start = SDL_GetTicks();
    while (SDL_GetTicks() - start < milliseconds) {
        if (LCDRenderShouldClose())
            return;
        SDL_Delay(1);
    }
}
```

It cannot move here as written: it makes the lowest layer depend on SDL2 **and** on the LCD renderer,
which would drag both into fs.ll's standalone build. So the Simulator `Delay` in this library is a plain
`nanosleep`.

Nothing is lost in terms of event handling — gui.ll's Simulator runs its own SDL loop on a dedicated
thread, which does the `SDL_PumpEvents` / `SDL_PollEvent` work. The only behaviour dropped is the early
abort: closing the window during a `Delay(1000)` is noticed up to a second later.

The dev wants to think about a transparent way to bring it back. Sketch of the shape it would need: hal.ll
would have to accept an optional "should I stop waiting?" predicate that a higher layer registers, so the
dependency points the right way — something like `DelaySetInterruptHandler(bool (*shouldStop)(void))`,
with gui.ll registering `LCDRenderShouldClose`. Not implemented, not decided.
