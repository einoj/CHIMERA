#include <avr/io.h>
#include <stdint.h>

// ATMega128 GPIO Port initialization
void PORT_Init() {
    DDRA=0xFF;
    PORTA=0xFF; // CS1-8 High
    
    DDRB=0xF7;
    PORTB=0x80; // VCC_EN12 high

    DDRC=0xFF;
    PORTC=0x1F; // VCC EN1-5 high, CS 10-12, low
    
    DDRD=0xF0;
    PORTD=0xF0; // VCC EN8-11 high
    
    DDRE=0x01; 
    PORTE=0x01;
    
    DDRF=0x00;
    PORTF=0x00;
    
    DDRG=0xFF;
    PORTG=0x03; // VCC EN6-7 high, CS9 low
}
