#include <avr/io.h>
#define F_CPU 11059200UL
#include <util/delay.h>
#include <avr/interrupt.h>
#define USART_BAUDRATE 115200
#define serBAUD_DIV_CONSTANT			( ( unsigned long ) 16 )
/* Constants for writing to UCSR0B. */
#define serRX_INT_ENABLE				( ( unsigned char ) 0x80 )
#define serRX_ENABLE					( ( unsigned char ) 0x10 )
#define serTX_ENABLE					( ( unsigned char ) 0x08 )
#define serTX_INT_ENABLE				( ( unsigned char ) 0x20 )
/* Constants for writing to UCSR0C. */
#define serUCSR0C_SELECT					( ( unsigned char ) 0x80 )
#define serEIGHT_DATA_BITS				( ( unsigned char ) 0x06 )
#define UBRR_VALUE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

void USART0Init(void) {
unsigned long ulBaudRateCounter;
unsigned char ucByte;
    // Set baud rate
    //UBRR0H = (uint8_t)(UBRR_VALUE>>8);
    //UBRR0L = (uint8_t)UBRR_VALUE;
    ulBaudRateCounter = ( F_CPU / ( serBAUD_DIV_CONSTANT * USART_BAUDRATE) ) - ( unsigned long ) 1;

    /* Set the baud rate. */	
    ucByte = ( unsigned char ) ( ulBaudRateCounter & ( unsigned long ) 0xff );	
    UBRR0L = ucByte;

    ulBaudRateCounter >>= ( unsigned long ) 8;
    ucByte = ( unsigned char ) ( ulBaudRateCounter & ( unsigned long ) 0xff );	
    UBRR0H = ucByte;
    // Set frame format to 8 data bits, no parity, 1 stop bit
    //UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00);
    UCSR0C = ( serUCSR0C_SELECT | serEIGHT_DATA_BITS );
    //enable transmission and reception
    UCSR0B |= (1<<RXEN0)|(1<<TXEN0);
    UCSR0B |= 0x80;
}

void SPIInit(void) {
     // Set MOSI, SCK as Output
     DDRB=(1<<5)|(1<<3)|(1<<4);
 
    // Enable SPI, Set as Master
    // Prescaler: Fosc/16, Enable Interrupts
    //The MOSI, SCK pins are as per ATMega8
    SPCR=(1<<SPE)|(1<<MSTR)|(1<<SPR0)|(1<<SPIE);
}

unsigned char JEDEC_ID()
{
    // Load data into the buffer
    SPDR = 0x9F;
 
    //Wait until transmission complete
    while(!(SPSR & (1<<SPIF) ));
 
    // Return received data
    return(SPDR);
}

void USART0SendByte(uint8_t u8Data) {
    //wait while previous byte is completed
    while(!(UCSR0A&(1<<UDRE0))){};
    // Transmit data
    UDR0 = u8Data;
}
uint8_t USART0ReceiveByte() {
    // Wait for byte to be received
    while(!(UCSR0A&(1<<RXC0))){};
    // Return received data
    return UDR0;
}
int main (void) {
    uint8_t u8TXData;
    uint8_t u8RXData;
    uint8_t data[4] = {1,2,4,8};
    uint8_t i = 0;
    uint8_t dir = 0;

    //Initialize USART0
    USART0Init();
	//DDRB |= 0x0f;
    SPIInit();
    sei();

    while(1) {
    //UDR0 = 0x8C;//JEDEC_ID();


    }
}

ISR(USART0_RX_vect)
{
signed char cChar;

	/* Get the character and post it on the queue of Rxed characters.
	If the post causes a task to wake force a context switch as the woken task
	may have a higher priority than the task we have interrupted. */
	cChar = UDR0;
    UDR0 = cChar;

}
