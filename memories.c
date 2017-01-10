#include "memories.h"

// number of pages, AND THEN page size
const struct Memory mem_arr[12] = {
 {0b00000001, 0b00010000, 2048, 256, &PORTA, &PORTC, 1}, // LE25U40CMC
 {0b00000010, 0b00000100, 2048, 256, &PORTA, &PORTC, 1}, // LE25U40CMC
 {0b00000100, 0b00001000, 2048, 256, &PORTA, &PORTC, 1}, // LE25U40CMC
 {0b00001000, 0b00000010, 8, 256, &PORTA, &PORTC, 0},
 {0b00010000, 0b00000001, 2048, 256, &PORTA, &PORTC, 1},
 {0b00100000, 0b00000010, 8, 256, &PORTA, &PORTG, 0},
 {0b01000000, 0b00000001, 512, 256, &PORTA, &PORTG, 1}, // 23LC1024
 {0b10000000, 0b01000000, 512, 256, &PORTA, &PORTD, 1}, // 23LC1024
 {0b00000100, 0b00010000, 512, 256, &PORTG, &PORTD, 1}, // 23LC1024
 {0b10000000, 0b10000000, 128, 256, &PORTC, &PORTD, 0},
 {0b01000000, 0b00100000, 128, 256, &PORTC, &PORTD, 0},
 {0b00100000, 0b10000000, 128, 256, &PORTC, &PORTB, 0},
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
