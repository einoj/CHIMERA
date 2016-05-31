#define PIN_ADC_VCC 3
#define PIN_ADC_SPI 3

#ifdef ATMEGA128S
  #define PORT_ADC PORTF
#else
  #define PORT_ADC PORTA
#endif
