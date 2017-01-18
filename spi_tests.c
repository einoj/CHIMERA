#include <stdint.h>
#include "kiss_tnc.h"
#include "memories.h"
#include "spi_memory_driver.h"


// That is the new code i was using to test the new boards

uint8_t test_CHIMERA_v2_memory0(void) {


    uint8_t buf[256];
    uint8_t pattern[2] = {0x55,0xAA};

    enable_memory_vcc(mem_arr[0]);

    //erase chip
    while (erase_chip(0) == BUSY);

    // read page	
    while (read_24bit_page(0, 0, buf) == BUSY);

    // check that buffer is 0;
    for (uint16_t i; i < 256; i++) {
        if (buf[i] != 0xff) {
            Send_NACK();
            disable_memory_vcc(mem_arr[0]);
            return -1;
        }
    }
    Send_ACK();

    // Page program first page of memory 0
    for (uint16_t i = 0; i < mem_arr[0].page_num; i++) {	
        while (write_24bit_page(i*mem_arr[0].page_size, 0, 0) == BUSY);
    }

    // read page	
    for (uint16_t k = 0; k < mem_arr[0].page_num; k++) {	
        while (read_24bit_page(k*mem_arr[0].page_size, 0, buf) == BUSY);

        // check that buffer contains the correct pattern;
        uint8_t pattern_idx = 0;
        for (uint16_t i; i < 256; i++) {
            if (buf[i] != pattern[pattern_idx]) {
                Send_NACK();
                disable_memory_vcc(mem_arr[0]);
                return -1;
            }
            pattern_idx ^= 1;
        }
    }

    Send_ACK();
    disable_memory_vcc(mem_arr[0]);
return 0;
}
