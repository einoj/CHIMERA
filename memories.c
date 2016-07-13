#include "memories.h"
static const mem memory_devices[] = {
 {0b00000001, 0b00010000, 1024, 256, &PORTA, &PORTC},
 {0b00000010, 0b00000100, 1024, 256, &PORTA, &PORTC},
 {0b00000100, 0b00001000, 1024, 256, &PORTA, &PORTC},
 {0b00001000, 0b00000010, 1024, 256, &PORTA, &PORTC},
 {0b00010000, 0b00000001, 1024, 256, &PORTA, &PORTC},
 {0b00100000, 0b00000010, 1024, 256, &PORTA, &PORTG},
 {0b01000000, 0b00000001, 1024, 256, &PORTA, &PORTG},
 {0b10000000, 0b01000000, 1024, 256, &PORTA, &PORTD},
 {0b00000100, 0b00010000, 1024, 256, &PORTG, &PORTD},
 {0b10000000, 0b10000000, 1024, 256, &PORTC, &PORTD},
 {0b01000000, 0b00100000, 1024, 256, &PORTC, &PORTD},
 {0b00100000, 0b10000000, 1024, 256, &PORTC, &PORTB},
};

/* sets the direction of memory chip select and VCC pins
 * also initializes the chip select and VCC pins to 0 */
void init_memories(void) {

}
