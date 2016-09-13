#include <avr/io.h>
#include <stdint.h>

// ATMega128 GPIO Port initialization
void PORT_Init() {
	DDRA=0xFF;
	PORTA=0x00;
	
	DDRB=0xFF;
	PORTB=0x80;

	DDRC=0xFF;
	PORTC=0x1F;
	
	DDRD=0xFF;
	PORTD=0xF0;
	
	DDRE=0x01;
	PORTE=0x01;
	
	DDRF=0x00;
	PORTF=0x00;
	
	DDRG=0xFF;
	PORTG=0x03;
}