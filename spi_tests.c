#include "kiss_tnc.h"
// That is the new code i was using to test the new boards

uint8_t test_CHIMERA_v2(void) {
    // Disable all CS
    for (uint8_t i = 0; i < 12; i++)CHIP_DESELECT(i);

    // VCC enable all memories
    for (uint8_t i = 0; i < 12; i++) {
        //enable_pin_macro(*mem_arr[i].cs_port, mem_arr[i].PIN_CS);
        //enable_memory_vcc(mem_arr[i]);
        disable_memory_vcc(mem_arr[i]);
    }

    enable_memory_vcc(mem_arr[0]);

    uint8_t buf[256];
    uint8_t pattern[2] = {0x55,0xAA};

    //erase chip
    while (erase_chip(0) == BUSY);

    // read page	
    while (read_24bit_page(256, 0, buf) == BUSY);

    // check that buffer is 0;
    for (uint16_t i; i < 256; i++) {
        if (buf[i] != 0) {
            Send_NACK();
            return -1;
        }
    }
    Send_ACK();

    // Page program first page of memory 0
    while (write_24bit_page(0, 0, 0) == BUSY);

    // read page	
    while (read_24bit_page(256, 0, buf) == BUSY);

    // check that buffer contains the correct pattern;
    uint8_t pattern_idx = 0;
    for (uint16_t i; i < 256; i++) {
        if (buf[i] != pattern[pattern_idx]) {
            Send_NACK();
            return -1;
        }
        pattern_idx ^= 1;
    }
    Send_ACK();
    return 0;
}
