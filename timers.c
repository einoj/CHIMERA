#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>

#include "main.h"
#include "timers.h"

// Timer 0 Initialization
// Parser timer
void TIMER0_Init() {
	TCCR0=0x00; // clock off at the beginning
	TIMSK|=0x01; // overflow interrupt ON
	TCNT0=0xFF-CHI_PARSER_TIMEOUT; // 157 (TBD) ticks with prescaler 10254 needed to generate 20ms time-out (TBD)
}

// Timer 1 Initialization
// Instrument clock timer
void TIMER1_Init() {
	TCCR1A=0x00;
	TCCR1B=0x03; // prescaler %64, fin=8000000
	TCCR1C=0x00;
	TIMSK|=0x04;
	TCNT1=0xFFFF-125; // We need 125 ticks to get 1ms interrupt
}

// Timer 3 Initialization
// SPI Timeout timer
// Interrupt is executed only when there is timeout
void TIMER3_Init() {
	TCCR3A=0x00;
	TCCR3B=0x05; // prescaler %1024, fin=8000000
	TCCR3C=0x00;
	ETIMSK|=0x00; // Don't enable on startup, old: 0x04
	TCNT3=0xFFFF-7812; // We need 7812 ticks to get 1s interrupt
}

void TIMER3_Enable_1s() {
    CHI_Board_Status.SPI_timeout_detected=0;
	TCNT3=0xFFFF-7812; // We need 7812 ticks to get 1s interrupt
	ETIMSK|=0x04; // Don't enable on startup
}

void TIMER3_Enable_8s() {
    CHI_Board_Status.SPI_timeout_detected=0;
	TCNT3=0xFFFF-62500; // We need 62500 ticks to get 8s interrupt
	ETIMSK|=0x04; // Don't enable on startup
}

void TIMER3_Disable() {
	ETIMSK &= ~ 0x04; // Don't enable on startup
}
