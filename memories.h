#include <avr/io.h>
/* This file defines the pins used by the memories and functions used to initialize the memories.*/

void init_memories(void);

void poweroff_memories(void);

/* Using these macros to enable and disable the chipselect 
 is more efficent than using a function because it avoids the
 extra LD and ST instructions see http://www.atmel.com/webdoc/AVRLibcReferenceManual/FAQ_1faq_port_pass.html
 Usage example: 
    static const mem mem_dev = {1,1024,256,&PORTB};
    enable_cs_macro (*mem_dev.port, mem_dev.PIN_CS);
*/
#define enable_pin_macro(port,mask) ((port) |= (mask))
#define disable_pin_macro(port,mask) ((port) &= ~(mask))
#define CHIP_DESELECT(i) ((*mem_arr[i].cs_port) |= (mem_arr[i].PIN_CS))
#define CHIP_SELECT(i) ((*mem_arr[i].cs_port) &= ~(mem_arr[i].PIN_CS))

//void chip_select(uint8_t i);
//void chip_deselect(uint8_t i);

struct Memory {
    // chip select pin
    uint8_t PIN_CS;
    // chip VCC enable pin
    uint8_t PIN_VCC;
    // number of pages in device
    const uint32_t page_num;
    // size of one page in bytes
    const uint32_t page_size;
    // The port that the memory chip select is connected to
    volatile uint8_t *cs_port;
    // The port for controlling memory VCC
    volatile uint8_t *vcc_port;
    // 0 is 16 bit address 1 is 24 bit address
    const uint8_t addr_space;
    // 0 not sram 1 is sram
    const uint8_t sram;
};

const struct Memory mem_arr[12];

