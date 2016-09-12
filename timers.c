#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>

#include "main.h"
#include "timers.h"

// Timer 1 Initialization
void TIMER1_Init() {
	TCCR1A=0x00;
	TCCR1B=0x03; // prescaler %64, fin=8000000
	TCCR1C=0x00;
	TIMSK|=0x04;
	TCNT1=0xFFFF-125; // We need 125 ticks to get 1ms interrupt
}

// Timer 3 Initialization
void TIMER3_Init() {
	TCCR3A=0x00;
	TCCR3B=0x07; // prescaler %1024, fin=8000000
	TCCR3C=0x00;
	ETIMSK|=0x04;
	TCNT3=0xFFFF-7812; // We need 7812 ticks to get 1s interrupt
}
