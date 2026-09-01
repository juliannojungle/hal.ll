# hal.ll.cmake
#
# Copy this file anywhere into your project and include() it from your CMakeLists.txt,
# after project(). It locates hal.ll through HAL_LL_PATH, downloading the project when
# that directory is not populated yet.
#
# HAL_LL_PATH is cached, so several submodules of the same project (each one carrying
# its own copy of this file) share a single hal.ll checkout: the first to resolve wins.
#
# Inputs:
#   HAL_LL_PATH    - path to the hal.ll root directory (variable or environment).
#                    Relative paths are resolved against CMAKE_SOURCE_DIR.
#                    Defaults to a 'hal.ll' folder next to this file.
#   PLATFORM_NAME  - Simulator (default), RP2040 or ESP32.
#
# Outputs:
#   HAL_LL_PATH         - cached, absolute path to the hal.ll root directory.
#   HAL_LL_PLATFORM_DIR - the resolved platform folder.
#   SOURCES             - appended with the hal.ll sources.
#   INCLUDE_DIRS        - appended with the hal.ll include directories.
#
# This file only sets variables. It must never call directory- or target-scoped
# commands such as add_compile_definitions: ESP-IDF evaluates the component that
# includes it in script mode (cmake -P), where those commands do not exist.

if(DEFINED ENV{HAL_LL_PATH} AND (NOT HAL_LL_PATH))
    set(HAL_LL_PATH $ENV{HAL_LL_PATH})
    message("Using HAL_LL_PATH from environment ('${HAL_LL_PATH}')")
endif()

if(NOT HAL_LL_PATH)
    set(HAL_LL_PATH "${CMAKE_CURRENT_LIST_DIR}/hal.ll")
endif()

get_filename_component(HAL_LL_PATH "${HAL_LL_PATH}" REALPATH BASE_DIR "${CMAKE_SOURCE_DIR}")

# Sentinel file used to tell a populated checkout from an empty/missing directory.
set(HAL_LL_SENTINEL_FILE "${HAL_LL_PATH}/src/lib/Types.h")

if(NOT EXISTS "${HAL_LL_SENTINEL_FILE}")
    find_package(Git QUIET)
    if(NOT Git_FOUND)
        message(FATAL_ERROR
            "hal.ll was not found at '${HAL_LL_PATH}' and git is not available to download it. "
            "Please install git or set HAL_LL_PATH to an existing hal.ll checkout.")
    endif()

    message("Downloading hal.ll into '${HAL_LL_PATH}'")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" clone --branch main --depth 1
                https://github.com/juliannojungle/hal.ll.git "${HAL_LL_PATH}"
        RESULT_VARIABLE HAL_LL_CLONE_RESULT
        ERROR_VARIABLE HAL_LL_CLONE_ERROR)

    if(NOT HAL_LL_CLONE_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to download hal.ll into '${HAL_LL_PATH}': ${HAL_LL_CLONE_ERROR}")
    endif()

    if(NOT EXISTS "${HAL_LL_SENTINEL_FILE}")
        message(FATAL_ERROR "Directory '${HAL_LL_PATH}' does not appear to contain hal.ll")
    endif()
endif()

set(HAL_LL_PATH "${HAL_LL_PATH}" CACHE PATH "Path to the hal.ll root directory" FORCE)

if(NOT DEFINED PLATFORM_NAME)
    set(PLATFORM_NAME "Simulator")
endif()

set(HAL_LL_LIB_DIR "${HAL_LL_PATH}/src/lib")
set(HAL_LL_PLATFORM_DIR "${HAL_LL_LIB_DIR}/Platform/${PLATFORM_NAME}")

if(NOT EXISTS "${HAL_LL_PLATFORM_DIR}")
    message(FATAL_ERROR "hal.ll has no support for platform '${PLATFORM_NAME}' ('${HAL_LL_PLATFORM_DIR}' not found)")
endif()

set(SOURCES
    ${SOURCES}
    "${HAL_LL_PLATFORM_DIR}/HAL.c")

set(INCLUDE_DIRS
    ${INCLUDE_DIRS}
    "${HAL_LL_LIB_DIR}"
    "${HAL_LL_LIB_DIR}/Helper"
    "${HAL_LL_PLATFORM_DIR}")

# The consumer links these into its own target. Touching the peripherals means
# depending on the platform SDK, and the consumer cannot be expected to know which
# parts of it this library reaches for. Not needed on ESP32: the component's
# REQUIRES covers it.
if(PLATFORM_NAME STREQUAL "RP2040")
    set(PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES}
        pico_stdlib pico_multicore hardware_spi hardware_gpio hardware_pwm
        hardware_uart hardware_rtc)
elseif(PLATFORM_NAME STREQUAL "Simulator")
    set(PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} pthread)
elseif(PLATFORM_NAME STREQUAL "ESP32")
    # The consumer passes these to idf_component_register's REQUIRES.
    set(PLATFORM_REQUIRES ${PLATFORM_REQUIRES}
        driver esp_system esp_timer esp_driver_uart esp_driver_ledc esp_driver_gpio)
endif()

# Guards against the same hal.ll being included by more than one sibling library.
list(REMOVE_DUPLICATES SOURCES)
list(REMOVE_DUPLICATES INCLUDE_DIRS)
if(PLATFORM_LIBRARIES)
    list(REMOVE_DUPLICATES PLATFORM_LIBRARIES)
endif()
if(PLATFORM_REQUIRES)
    list(REMOVE_DUPLICATES PLATFORM_REQUIRES)
endif()
