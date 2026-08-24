# Host-side unit tests for the MFRC522 driver.
#
# The core library is hardware-independent, so its register driver, transports
# and CRC logic can be tested on the host using a mocked MFRC522 (a register
# file + FIFO emulated per transport). Enable with -DMFRC522_BUILD_TESTS=ON.

# Transport / register-driver / CRC tests.
add_executable(test_transport test_transport.c)
target_link_libraries(test_transport PRIVATE mfrc522_core)
target_include_directories(test_transport PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_compile_options(test_transport PRIVATE -std=c99 -Wall -Wextra)
add_test(NAME transport COMMAND test_transport)

# STM32 adapter wiring test (uses the HAL stub; no hardware required).
if(MFRC522_BUILD_STM32)
    add_executable(test_stm32_adapter test_stm32_adapter.c
                   stm32_hal_stub/stm32h7xx_hal.c)
    target_link_libraries(test_stm32_adapter PRIVATE mfrc522_core mfrc522_stm32)
    target_include_directories(test_stm32_adapter PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../platform/stm32
        ${CMAKE_CURRENT_SOURCE_DIR}/stm32_hal_stub
    )
    target_compile_definitions(test_stm32_adapter PRIVATE STM32H743xx)
    target_compile_options(test_stm32_adapter PRIVATE -std=c99 -Wall -Wextra)
    add_test(NAME stm32_adapter COMMAND test_stm32_adapter)
endif()
