#include <avr/io.h>
#include <stdio.h>
//#define F_CPU 11059200UL
#define F_CPU 8000000UL
#include <util/delay.h>
#include <avr/interrupt.h>
#include "spi_memory_driver.h"
#include "data_packet.h"
#include "hex_inv.h"
#define USART_BAUDRATE 9600
#define serBAUD_DIV_CONSTANT			( ( unsigned long ) 16 )
/* Constants for writing to UCSR0B. */
#define serRX_INT_ENABLE				( ( unsigned char ) 0x80 )
#define serRX_ENABLE					( ( unsigned char ) 0x10 )
#define serTX_ENABLE					( ( unsigned char ) 0x08 )
#define serTX_INT_ENABLE				( ( unsigned char ) 0x20 )
/* Constants for writing to UCSR1C. */
#define serEIGHT_DATA_BITS				( ( unsigned char ) 0x06 )
#define UBRR_VALUE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

data_packet data_arr[2];
uint8_t write_i = 0 ; // points at which packet in the data array is being written to
uint8_t read_i = 1; // points at which packet in the data array is beaing read and sent to the OBC


void USART0Init(void) {
unsigned long ulBaudRateCounter;
unsigned char ucByte;
    // Set baud rate
    //UBRR0H = (uint8_t)(UBRR_VALUE>>8);
    //UBRR0L = (uint8_t)UBRR_VALUE;
    ulBaudRateCounter = ( F_CPU / ( serBAUD_DIV_CONSTANT * USART_BAUDRATE) ) - ( unsigned long ) 1;

    /* Set the baud rate. */	
    ucByte = ( unsigned char ) ( ulBaudRateCounter & ( unsigned long ) 0xff );	
    UBRR1L = ucByte;

    ulBaudRateCounter >>= ( unsigned long ) 8;
    ucByte = ( unsigned char ) ( ulBaudRateCounter & ( unsigned long ) 0xff );	
    UBRR1H = ucByte;
    
    // Set frame format to 8 data bits, no parity, 1 stop bit
    //UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00);
    //UCSR1C = (serEIGHT_DATA_BITS );
    //enable transmission and reception
    UCSR0B |= (1<<RXEN0)|(1<<TXEN0);
    UCSR0B |= 0x80; //This also sets the RXEN1 bit
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

static void printuart(char *msg) {
    while (*msg != '\0') {
        USART0SendByte(*msg);
        *msg++;
    }
}


//ISR(PCINT14_vect) {
//    SELECT_SERIAL_MEMORY;
//    if (byte_cnt == nb_byte) {
//        spi_tx_byte(AAI);
//        data_ptr++;          
//        spi_tx_byte(*data_ptr);
//        data_ptr++;          
//        spi_tx_byte(*data_ptr);
//    } else {
//        spi_tx_byte(WRDI);
//        DESELECT_SERIAL_MEMORY;
//        SELECT_SERIAL_MEMORY;
//        spi_tx_byte(DBSY);
//    }
//    DESELECT_SERIAL_MEMORY;
//}

//uint8_t write_aai(uint32_t start_addr, uint8_t n_bytes, uint8_t *src) {
//    uint8_t status_reg;
//    if (start_addr <= TOP_ADDR) {
//        if (read_status_reg(&status_reg) == TRANSFER_COMPLETED) {// Is the SPI interface being used?
//            if (!(status_reg & (1<<WIP))) { // Is a write in progress internally on the memory?
//                // TODO should test for write protectioin
//                ENABLE_MISO_INTERRUPT; //the MISO line will be pulled high when a write command is finished
//                write_spi_command(EBSY); // configures the Serial Output (SO) pin to indicate Flash Busy status
//                write_spi_command(WREN); // Write enable always has to be sent before a write operation.
//                DISABLE_SPI_INTERRUPT;
//                // Seeting the state to INSTRUCTION will cause the spi interrupt to send the first
//                // byte of the three byte address.
//                state = ADDRESS; 
//                // global variables used by interrupt function
//                nb_byte = n_bytes; 
//                byte_cnt = 2;
//                address = start_addr;
//                data_ptr = src;
//                SELECT_SERIAL_MEMORY;
//                // SPDR = AAI; // write byte program command to SPI
//                // Now send Address address address data data
//                spi_tx_byte(AAI)
//                spi_tx_byte((uint8_t)(address>>16);
//                spi_tx_byte((uint8_t)(address>>8);
//                spi_tx_byte((uint8_t)(address);
//                spi_tx_byte(*data_ptr); // send first half of word
//                data_ptr++;          
//                spi_tx_byte(*data_ptr); // send second half of word
//                // Next word will be sent when MOSI interrupt is received 
//                DESELECT_SERIAL_MEMORY;
//                return TRANSFER_STARTED;
//            } else {
//                return BUSY;
//            }
//        } else {
//            return BUSY;
//        }
//    } else {
//        return OUT_OF_RANGE;
//    }
//}



void USART0_Init(void ){
    //UBRR0L = (unsigned char) (ubrr>>8);
    //UBRR0H = (unsigned char) ubrr;
    UCSR0B = (1<<TXEN);
    //UCSR0C = (3<<UCSZ0);
    //
    UBRR0L= 12; // 8MHz internal clock, 38.4kbit
	UBRR0H= 0;
	UCSR0B=0b10011000;//((1<<RXEN0)|(1<<TXEN0));
    //UCSR0B = (1<<TXEN);
    UCSR0C = (3<<UCSZ0);
	//UCSR0C=0b00000110;
}
/*
int main (void) {
   // DDRB = 0xff;
   // PORTB = 0xff;
    //USART0Init();
    //sei();
    //USART0SendByte(0xAA);
    //USART0SendByte(0x55);
    //USART0SendByte(0xAA);
    //USART0SendByte(0x55);
    
    // Set frame format to 8 data bits, no parity, 1 stop bit
    //UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00);
    //UCSR1C = (serEIGHT_DATA_BITS );
    //enable transmission and reception
    //UCSR0B |= (1<<RXEN0)|(1<<TXEN0);
    //UCSR0B |= 0x80; //This also sets the RXEN1 bit

    USART_Init();

    while (1) USART0SendByte(0x56);
    //wait while previous byte is completed
   // char msg[13] = "Hello World!";
   // while (1) {
   //     printuart(msg);
   // }
    //printuart("Y\r\n");
//    uint16_t i = 0;
//    DDRB = 0xff;
//    PORTB = 0b00010000;
//    uint32_t error_cnt;
//    uint8_t JEDEC_ID;
//    uint8_t reg_status;
//    uint32_t tmp_addr;
//    static uint8_t dest[257*sizeof(uint8_t)];
//   // static uint8_t src[10];
//    // initialize the data packet indexes
//    data_arr[0].index = 0;
//    data_arr[1].index = 0;
//
//   // src[0] = 0x55;
//   // src[1] = 0xAA;
//   // src[2] = 0x55;
//   // src[3] = 0xAA;
//   // src[4] = 0x55;
//   // src[5] = 0xAA;
//   // src[6] = 0x55;
//   // src[7] = 0xAA;
//   // src[8] = 0x55;
//   // src[9] = 0xAA;
//
//    //Initialize USART0
//	//DDRB |= 0x0f;
//    spi_init();
//    sei();
//
//    get_jedec_id(&JEDEC_ID);
//
//    if (JEDEC_ID == 0x25) {
//        printuart("Y\r\n");
//    } else {
//        printuart("N\r\n");
//    }
//
//    char msg[256];
//    uint32_t page_num = 0xfffff+1;
//    error_cnt = 0;
//    page_num = page_num/255+1;
//    write_status_reg(0x00);
//    //erase_chip();
//    read_status_reg(&reg_status);
//    sprintf(msg,"reg status: 0x%02x\r\n",reg_status);
//    printuart(msg);
//    //while(write_byte_array(6,1,src) != TRANSFER_STARTED);
//    //while(aai_pattern() != TRANSFER_COMPLETED);
//    printuart("STARTING MEMORY CHECK!\r\n");
//    hex_init();
//    toggle_hex(VCC_EN1); 
//    for (tmp_addr = 0; tmp_addr < 0xfffff; tmp_addr+=256) {
//        while(read_byte_arr(tmp_addr,256,dest) != TRANSFER_COMPLETED);
//            for (i = 0; i < 255; i++) {
//                if ((i & 1) && (dest[i] != 0xaa)) {
//                    //odd
//                    error_cnt++;
//                    sprintf(msg, "Erro: addr %d should be 0xaa is %02x\r\n", i, dest[i]);
//                    put_data(&data_arr[write_i], i, 0x5, ADDR_SEU_AA);
//                    printuart(msg);
//                } else if ( !(i & 1) && (dest[i] != 0x55)) {
//                    //even
//                    error_cnt++;
//                    sprintf(msg, "Erro: addr %d should be 0xaa is %02x\r\n", i, dest[i]);
//                    put_data(&data_arr[write_i], i, 0x5, ADDR_SEU_55);
//                    printuart(msg);
//            } 
//        }
//    }
//    toggle_hex(VCC_EN1); 
//    send_packet(&data_arr[write_i]);
//
//    //send_packet(&data_arr[read_i]);
//    //while (addr<0xfffff){
//    //    read_byte_arr(addr,255,dest);
//    //    addr += 255;
//    //    for (i = 0; i < 255; i++) {
//    //        if ((i & 1) && (dest[i] != 0xaa)) {
//    //            //odd
//    //            error_cnt++;
//    //            sprintf(msg, "Erro: addr %d should be 0xaa is %d\r\n", i, dest[i]);
//    //            printuart(msg);
//    //        } else if ( !(i & 1) && (dest[i] != 0x55)) {
//    //            error_cnt++;
//    //            sprintf(msg, "Erro: addr %d should be 0xaa is %d\r\n", i, dest[i]);
//    //            printuart(msg);
//    //        } 
//    //    }
//    //    page_cnt++;
//    //    sprintf(msg, "Checked page %lu of %lu\r\n", page_cnt, page_num);
//    //    printuart(msg);
//    //}
//    printuart("DONE!\r\n");
//    sprintf((char *)msg, "Detected %lu errors\r\n", error_cnt);
//    printuart(msg);
//
//    while(1) {
//    //UDR0 = 0x8C;//JEDEC_ID();
//
//
//    }
}
*/

/*k
ISR(USART0_RX_vect)
{
signed char cChar;

	//Get the character and post it on the queue of Rxed characters.
	//If the post causes a task to wake force a context switch as the woken task
	//may have a higher priority than the task we have interrupted. 
	cChar = UDR0;
    UDR0 = cChar;

}
*/
