#include <avr/io.h>

void USART0SendByte(uint8_t u8Data) {
  //wait while previous byte is completed
  while((UCSR0A & 1<<UDRE0) == 0) ;
  // Transmit data
  UDR0 = u8Data;
}

uint8_t USART0ReceiveByte() {
  // Wait for byte to be received
  while(!(UCSR0A&(1<<RXC0))){};
  // Return received data
  return UDR0;
}

void USART0_Init(void ){
  UCSR0B = (1<<TXEN);
  UBRR0L= 12; // 8MHz internal clock, 38.4kbit UART speed
  UBRR0H= 0;
  UCSR0B=0b10011000;//((1<<RXEN0)|(1<<TXEN0));
  UCSR0C = (3<<UCSZ0);
}
