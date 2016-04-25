#include <avr/io.h>
#include <stdio.h>
#define F_CPU 11059200UL
#include <util/delay.h>
#include <avr/interrupt.h>
#include "spi_memory_driver.h"
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

void USART1Init(void) {
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
    UCSR1C = ( serUCSR0C_SELECT | serEIGHT_DATA_BITS );
    //enable transmission and reception
    UCSR1B |= (1<<RXEN0)|(1<<TXEN0);
    UCSR1B |= 0x80;
}

void spi_init(void) {
    volatile uint8_t IOreg;

    SPCR = (0<<SPE);  // Disable the SPI to be able to configure the #SS line as an input even if the SPI is configured as a slave
    // Set MOSI, SCK , and SS as Output
    DDRB=(1<<5)|(1<<7)|(1<<4);
    DESELECT_SERIAL_MEMORY;

    // Enable SPI, Set as Master
    // Prescaler: Fosc/16, Enable Interrupts
    //The MOSI, SCK pins are as per ATMega8
    SPCR=(1<<SPE)|(1<<MSTR)|(1<<SPIE);

    // Clear the SPIF flag
    IOreg = SPSR;
    IOreg = SPDR;

    state = READY_TO_SEND;

   sei();
}

 
/*! \brief  the SPI  Transfer Complete Interrupt Handler
 *
 * The SPI interrupt handler manages the accesses to the I/O Reg SPDR during a memory write cycle.
 * The values of the global variable(state,  byte_cnt, address, data_ptr) enables the handler to compute the next byte to be send
 * over the SPI interface as well as the next state. The finite state machine diagram is provided in the application note document.  
 *
 *  \return  void.
 */
ISR(SPI_STC_vect)
{
  switch (state) {
    ///////////////////////////////
    // INSTRUCTION STATE
    //////////////////////////////
    case INSTRUCTION :
    {
      state = ADDRESS;        
      byte_cnt = 1;
     SPDR = (uint8_t)(address>>16);      // Address phase is 3 byte long for SPI flashes
     break;
    }

    ///////////////////////////////
    // ADDRESS STATE
    //////////////////////////////
    case ADDRESS :
    {
      if (byte_cnt == NB_ADDR_BYTE) {    // is the last address byte reached?
        state = DATA;          // go to the DATA state
        byte_cnt = 0;
        SPDR = *data_ptr;     // send the first byte
      } else if (byte_cnt == 1) {    // must the middle address byte be sent?
        byte_cnt ++;
        SPDR = (uint8_t)(address>>8);
      } else {
        byte_cnt ++;
        SPDR = (uint8_t)(address);
      }
      break;
    } 

    ///////////////////////////////
    // DATA STATE
    //////////////////////////////
    case DATA :
    {
      data_ptr++;                 // point to the next byte (even if it was the last)  
      if (byte_cnt == nb_byte ) {  // is the last byte sent?
        DESELECT_SERIAL_MEMORY;   // Pull high the chip select line of the SPI serial memory                   // terminate current write transfer
        state = READY_TO_SEND;    // return to the idle state
      } else {      
        byte_cnt ++;
        SPDR = *data_ptr;
      }
      break;
    }
    
    default :
    {
      state = READY_TO_SEND;
    }
  }  
}


uint8_t spi_tx_byte(volatile uint8_t byte) {
    SPDR = byte;
    while (!(SPSR & (1<<SPIF)));
    return SPDR;
}

uint8_t get_status_reg(uint8_t *status) {
    if (state == READY_TO_SEND) {
        DISABLE_SPI_INTERRUPT;        
        SELECT_SERIAL_MEMORY;        // Pull down chip select.
        spi_tx_byte(RDSR);           // Send Read status register opcode.
        *status = spi_tx_byte(0xFF); // get the status register value, by sending 0xFF we avoid toggling the MOSI line.
        DESELECT_SERIAL_MEMORY;
        ENABLE_SPI_INTERRUPT;

        return TRANSFER_COMPLETED;
    } else {
        *status = 1;
        return BUSY;
    }

}

/**
 * Reads one or more bytes from the SPI Memory
 */
uint8_t read_byte_arr(uint32_t addr, uint32_t n_bytes, uint8_t *dest) {
    uint8_t status_reg;
    uint8_t access_status;
    uint8_t read_cnt;
    uint8_t *curr_dest;    // temporary variable for incrementing destination pointer

    curr_dest = dest;
    if ((addr) <= TOP_ADDR) {
        if(get_status_reg(&status_reg) == TRANSFER_COMPLETED) {
            if(!(status_reg & (1<<WIP))) { // Check if the memory is ready to be used. WIP is set an internal write process is in progress
                DISABLE_SPI_INTERRUPT;
                SELECT_SERIAL_MEMORY;
                spi_tx_byte(READ);
                spi_tx_byte((uint8_t)(addr>>16)); // Send the MSB byte. Casting to uint8_t will send onlyt the lower byte
                spi_tx_byte((uint8_t)(addr>>8));  // Send the middle byte
                spi_tx_byte((uint8_t)(addr));     // Send the LSB byte
                read_cnt = 0;

                while (read_cnt < n_bytes) {
                    *curr_dest++ = spi_tx_byte(0xFF);
                    read_cnt++;
                }
                DESELECT_SERIAL_MEMORY;
                ENABLE_SPI_INTERRUPT;
                access_status = TRANSFER_COMPLETED;
            } else {
                access_status = BUSY;
            }
        } else {
            access_status = BUSY;
        }
    } else {
        access_status = OUT_OF_RANGE;
    }
    return access_status;
}

/**
 * Writes a 4 byte pattern to the memory 
 */
//uint8_t write_pattern(uint32_t pattern) {
//    uint8_t status_reg;
//    if (get_status_reg(&status_reg) == TRANSFER_COMPLETED) {
//        if (!(status_reg & (1<<WIP))) { // Check if the memory is ready to be used. WIP is set an internal write process is in progress
//            // TODO implement check of write protect
//            // some memories have three BP bits, others two
//            spi_tx
//}


/** 
 * Get Memory JEDEC ID 
 * 
 * Returns the JEDEC ID of the serial memory.
 */
uint8_t get_jedec_id(uint8_t *memID)
{
    uint8_t status_reg;
    if (get_status_reg(&status_reg) == TRANSFER_COMPLETED)
    {
        if (!(status_reg & (1<<WIP))) // Check if the memory is ready to be used. WIP is set an internal write process is in progress
        { 
            DISABLE_SPI_INTERRUPT;
            SELECT_SERIAL_MEMORY;
            spi_tx_byte(JEDEC);    // Transmit JEDEC-ID opcode
            spi_tx_byte(0xff);     // Recieve manufacturer ID
            *memID = spi_tx_byte(0xff);     // Memory ID
            DESELECT_SERIAL_MEMORY;
            ENABLE_SPI_INTERRUPT;
            return TRANSFER_COMPLETED;
        } else {
            return BUSY;
        }
    } else {
        return BUSY;
    }
}

void USART1SendByte(uint8_t u8Data) {
    //wait while previous byte is completed
    while(!(UCSR1A&(1<<UDRE1))){};
    // Transmit data
    UDR1 = u8Data;
}
uint8_t USART1ReceiveByte() {
    // Wait for byte to be received
    while(!(UCSR1A&(1<<RXC1))){};
    // Return received data
    return UDR1;
}

static void printuart(int8_t *msg) {
    while (*msg != '\0') {
        USART1SendByte(*msg);
        *msg++;
    }
}

int main (void) {
    uint8_t u8TXData;
    uint8_t u8RXData;
    uint8_t data[4] = {1,2,4,8};
    uint16_t i = 0;
    uint8_t dir = 0;
    uint8_t JEDEC_ID;
    uint8_t reg_staus;
    static uint8_t dest[257*sizeof(uint8_t)];

    //Initialize USART0
    USART1Init();
	//DDRB |= 0x0f;
    spi_init();
    sei();

    get_jedec_id(&JEDEC_ID);

    if (JEDEC_ID == 0x25) {
        printuart("Y\r\n");
    } else {
        printuart("N\r\n");
    }

    uint32_t addr;
    uint8_t msg[256];
    uint32_t page_cnt = 0;
    uint32_t page_num = 0xfffff+1;
    page_num = page_num/255+1;
    //sprintf(msg, "Checked page %d of %d\r\n", page_cnt, page_num);
    //printuart(msg);
    printuart("STARTING MEMORY CHECK!\r\n");
    while (addr<0xfffff){
        read_byte_arr(0,255,dest);
        addr += 255;
        for (i = 0; i < 255; i++) {
            if ((i & 1) && (dest[i] != 0xaa)) {
                //odd
                sprintf(msg, "Erro: addr %d should be 0xaa is %d\r\n", i, dest[i]);
                printuart(msg);
            } else if ( !(i & 1) && (dest[i] != 0x55)) {
                sprintf(msg, "Erro: addr %d should be 0xaa is %d\r\n", i, dest[i]);
                printuart(msg);
            } 
        }
        page_cnt++;
        sprintf(msg, "Checked page %lu of %lu\r\n", page_cnt, page_num);
        printuart(msg);
    }
    printuart("DONE!\r\n");


    while(1) {
    //UDR0 = 0x8C;//JEDEC_ID();


    }
}

ISR(USART1_RX_vect)
{
signed char cChar;

	/* Get the character and post it on the queue of Rxed characters.
	If the post causes a task to wake force a context switch as the woken task
	may have a higher priority than the task we have interrupted. */
	cChar = UDR1;
    UDR1 = cChar;

}
