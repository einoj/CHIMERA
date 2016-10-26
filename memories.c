#include "memories.h"

// number of pages, AND THEN page size
const struct Memory mem_arr[12] = {
 {0b00000001, 0b00010000, 256, 4, &PORTA, &PORTC},
 {0b00000010, 0b00000100,   2, 4, &PORTA, &PORTC},
 {0b00000100, 0b00001000,   2,14, &PORTA, &PORTC},
 {0b00001000, 0b00000010, 256, 4, &PORTA, &PORTC},
 {0b00010000, 0b00000001, 256, 4, &PORTA, &PORTC},
 {0b00100000, 0b00000010, 256, 4, &PORTA, &PORTG},
 {0b01000000, 0b00000001, 256, 4, &PORTA, &PORTG},
 {0b10000000, 0b01000000, 256, 4, &PORTA, &PORTD},
 {0b00000100, 0b00010000, 256, 4, &PORTG, &PORTD},
 {0b10000000, 0b10000000, 256, 4, &PORTC, &PORTD},
 {0b01000000, 0b00100000, 256, 4, &PORTC, &PORTD},
 {0b00100000, 0b10000000, 256, 4, &PORTC, &PORTB},
};

/* sets the direction of memory chip select and VCC pins
 * also initializes the chip select and VCC pins to 0 */
void init_memories(void) {
}

/*
void chip_select(uint8_t i)
{
    *(mem_arr[i].cs_port) &= ~(mem_arr[i].PIN_CS);
}

void chip_deselect(uint8_t i) 
{
    *(mem_arr[i].cs_port) |= mem_arr[i].PIN_CS;
}
*/
