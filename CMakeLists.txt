# MFRC522 driver — build definition (Phase 1 skeleton).
#
# The core library compiles to a static library. The STM32 platform adapter is
# compiled separately (it depends on the STM32 HAL / CMSIS) and the examples
# link both. Building the adapter requires the STM32Cube HAL include paths and
# the device HAL sources; those are supplied by the embedding project (or by
# STM32CubeIDE), NOT by this CMakeLists.
#
#   cmake -S . -B build
#   cmake --build build

cmake_minimum_required(VERSION 3.16)
project(mfrc522 C)

# ---------------------------------------------------------------------------
# Core library (hardware independent)
# ---------------------------------------------------------------------------
add_library(mfrc522_core STATIC
    src/mfrc522.c
    src/mfrc522_registers.c
    src/mfrc522_crc.c
    src/mfrc522_irq.c
    src/mfrc522_protocol.c
    src/mfrc522_mifare.c
    interface/mfrc522_spi.c
    interface/mfrc522_i2c.c
    interface/mfrc522_uart.c
)

target_include_directories(mfrc522_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(mfrc522_core PUBLIC
    # Feature toggles are off-by-default in code; enable here if desired:
    # MFRC522_ENABLE_NONBLOCKING=1
)

# C99, extra warnings (helps keep the code MISRA-C-lean)
target_compile_options(mfrc522_core PRIVATE
    -std=c99
    -Wall -Wextra -Werror=implicit-function-declaration
)

# ---------------------------------------------------------------------------
# STM32 platform adapter (enabled by the embedding project, which must provide
# the HAL include directories and the device sources)
# ---------------------------------------------------------------------------
option(MFRC522_BUILD_STM32 "Build the STM32 HAL platform adapter" OFF)

if(MFRC522_BUILD_STM32)
    add_library(mfrc522_stm32 STATIC
        platform/stm32/mfrc522_stm32_spi.c
        platform/stm32/mfrc522_stm32_i2c.c
        platform/stm32/mfrc522_stm32_uart.c
        platform/stm32/mfrc522_stm32_gpio.c
        platform/stm32/mfrc522_stm32_time.c
    )
    target_include_directories(mfrc522_stm32 PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32
    )
    target_link_libraries(mfrc522_stm32 PUBLIC mfrc522_core)
    # The embedding project must add the HAL include dirs, e.g.:
    #   target_include_directories(mfrc522_stm32 PUBLIC ${STM32_HAL_DIR}/Inc)
endif()

# ---------------------------------------------------------------------------
# Host-side unit tests (no hardware required)
# ---------------------------------------------------------------------------
option(MFRC522_BUILD_TESTS "Build host-side tests" OFF)

if(MFRC522_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
