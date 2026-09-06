#[[
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
]]

set(PROJ_ROOT ${CMAKE_SOURCE_DIR})

# ESP-IDF may process this component in a scope where CMAKE_SOURCE_DIR is not the
# project root, so derive it from this file's own location as a fallback.
if(NOT EXISTS "${PROJ_ROOT}/hal.ll.cmake")
    get_filename_component(PROJ_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
endif()

set(PLATFORM_NAME "ESP32")

if(NOT HAL_LL_PATH)
    set(HAL_LL_PATH "${PROJ_ROOT}")
endif()

include(${PROJ_ROOT}/hal.ll.cmake)

idf_component_register(
    SRCS
        "${PROJ_ROOT}/src/Sample.c"
        ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES ${PLATFORM_REQUIRES})
