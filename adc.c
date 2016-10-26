#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>

#include "main.h"
#include "adc.h"
#include "CHIMERA_Board_Defs.h"

void ADC_Init() {
	
	ADMUX =  0b11101111;	// 01001=10x, 01011 = 200x
	ADCSRA = 0b11101101;	// Bits 2:0 , 100 = %16, 101 = %32, 110 = %64, 111 = %128
	// input freq: 8000000 , 14 cycles for conversation
}

ISR(ADC_vect)
{
	ADC_data = ADCL;
	ADC_data |= (ADCH<<8);
	
	ADC_data=ADC_data>>6;
	
	ADC_Samples[ADC_Sample_Cnt]=ADC_data;

	// Median procedure, mem effective, should be good..
	if (ADC_Samples[0]>ADC_Samples[1]) {
		if (ADC_Samples[0]>ADC_Samples[2]) {
			if (ADC_Samples[1]>ADC_Samples[2]) ADC_Median=ADC_Samples[1];
			else ADC_Median=ADC_Samples[2];
		}
		else ADC_Median=ADC_Samples[0];
	}
	else {
		if (ADC_Samples[1]>ADC_Samples[2]) {
			if (ADC_Samples[0]>ADC_Samples[2]) ADC_Median=ADC_Samples[0];
			else ADC_Median=ADC_Samples[2];
		}
		else ADC_Median=ADC_Samples[1];
	}

	ADC_Sample_Cnt=ADC_Sample_Cnt+1;
	if (ADC_Sample_Cnt>2) ADC_Sample_Cnt=0;
	
	if (ADC_Median>(signed short)0x01F0) {
		LDO_OFF;
		CHI_Board_Status.latch_up_detected=1;	

		// This might be verified as new board has no pull-ups, could be removed?
		//PORTB &= ~0x80;
		//PORTC &= ~0x1F;
		//PORTD &= ~0xF0;
		//PORTG &= ~0x03;
	}
}
