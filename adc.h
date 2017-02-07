// Interrupt: ADC Latch-Up monitoring
static volatile uint8_t ADC_Sample_Cnt;
static volatile int16_t ADC_data;
static volatile int16_t ADC_Samples[3];
volatile int16_t ADC_Median;

void ADC_Init();
