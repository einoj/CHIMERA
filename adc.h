// Interrupt: ADC Latch-Up monitoring
volatile uint8_t static ADC_Sample_Cnt;
volatile int16_t static ADC_data;
volatile int16_t static ADC_Samples[3];
volatile int16_t static ADC_Median;

void ADC_Init();