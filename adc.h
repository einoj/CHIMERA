#define PIN_ADC_VCC 3
#define PIN_ADC_SPI 2

#ifdef ATMEGA128S
  #define PORT_ADC PORTF
#else
  #define PORT_ADC PORTA
#endif

void ADC_Init() {
	ADMUX = 0b11001011; //ADC1 - ADC0
	ADCSRA = 0b11101100;
	//ADCSRA = 0b11101000;
	//PORTA=0xF0;
	//sei();
}

uint8_t static ADC_Sample_Cnt = 0;
int16_t static ADC_data;
int16_t static ADC_Samples[3];
int16_t static ADC_Median;
int16_t static ADC_mean;

void Reboot() {

}


// median
ISR(ADC_vect)
{	
	ADC_data = ADCL;
	ADC_data |= (ADCH<<8);
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
	//----
	ADC_Sample_Cnt=(ADC_Sample_Cnt+1) % 3;
}

//mean
ISR(ADC_vect)
{
    ADC_data = ADCL;
    ADC_data |= (ADCH<<8);
    ADC_Samples[ADC_Sample_Cnt] = ADC_data
    
    ADC_mean = (ADC_Samples[0]+ADC_Samples[1]+ADC_Samples[2])/3

}
