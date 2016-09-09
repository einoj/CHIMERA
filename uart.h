#define FOSC 1000000UL
#define BAUD 4800
#define MYUBRR FOSC/16/BAUD-1

void USART0_Init(void );

// TODO consider inlining
void USART0SendByte(uint8_t u8Data);
