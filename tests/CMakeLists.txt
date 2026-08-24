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
