# Host-side unit tests for the MFRC522 driver.
#
# The core library is hardware-independent, so its protocol, register-mapping
# and CRC logic can be tested on the host using a mocked platform/transport.
#
# (Populated in Phase 8. Enable with -DMFRC522_BUILD_TESTS=ON.)

# add_executable(test_crc test_crc.c ../src/mfrc522_crc.c)
# target_link_libraries(test_crc PRIVATE mfrc522_core)
# add_test(NAME crc COMMAND test_crc)
