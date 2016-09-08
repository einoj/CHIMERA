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
#define enable_cs_macro(port,mask) ((port) |= (mask))
#define disable_cs_macro(port,mask) ((port) ~= (mask))

typedef struct Memory {
    // chip select pin
    const uint8_t PIN_CS;
    // chip VCC enable pin
    const uint8_t PIN_VCC;
    // size of device in bytes 
    const uint32_t size;
    // size of one page in bytes
    const uint16_t page_size;
    // The port that the memory chip select is connected to
    volatile uint8_t *cs_port;
    // The port for controlling memory VCC
    volatile uint8_t *vcc_port;
} mem;

