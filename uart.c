#include <avr/io.h>
#include <stdio.h>
#define F_CPU 11059200UL
#include <util/delay.h>
#include <avr/interrupt.h>
#include "spi_memory_driver.h"
#include "data_packet.h"
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

data_packet data_arr[2];
uint8_t write_i = 0 ; // points at which packet in the data array is being written to
uint8_t read_i = 1; // points at which packet in the data array is beaing read and sent to the OBC


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
    SPCR = (0<<SPE);  // Disable the SPI to be able to configure the #SS line as an input even if the SPI is configured as a slave
    // Set MOSI, SCK , and SS as Output
    DDRB=(1<<MOSI)|(1<<SCK)|(1<<CS1);
    DESELECT_SERIAL_MEMORY;
    DISABLE_MISO_INTERRUPT; // To avoid triggering a write stop interrupt when reading

    // Enable SPI, Set as Master
    // Prescaler: Fosc/16, Enable Interrupts
    //The MOSI, SCK pins are as per ATMega8
    SPCR=(1<<SPE)|(1<<MSTR)|(1<<SPIE);

    // Clear the SPIF flag by reading SPSR and SPDR
    SPSR;
    SPDR;

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
  char msg[256];
  switch (state) {
    ///////////////////////////////
    // INSTRUCTION STATE
    //////////////////////////////
    case INSTRUCTION :
    {
      printuart("I\r\n");
      state = ADDRESS;        
      byte_cnt = 1;
      SPDR = (uint8_t)(address>>16);      // Address phase is 3 byte long for SPI flashes
      sprintf(msg, "address %02x\r\n",(uint8_t)(address>>16));
      printuart(msg);
     break;
    }

    ///////////////////////////////
    // ADDRESS STATE
    //////////////////////////////
    case ADDRESS :
    {
      if (byte_cnt == NB_ADDR_BYTE) {    // is the last address byte reached?
        printuart("B\r\n");
        state = DATA;          // go to the DATA state
        byte_cnt = 0;
        SPDR = *data_ptr;     // send the first byte
      sprintf(msg, "data %02x\r\n",*data_ptr);
      printuart(msg);
      } else if (byte_cnt == 1) {    // must the middle address byte be sent?
        printuart("C\r\n");
        byte_cnt ++;
        SPDR = (uint8_t)(address>>8);
      sprintf(msg, "address %02x\r\n",(uint8_t)(address>>8));
      printuart(msg);
      } else {
        printuart("D\r\n");
        byte_cnt ++;
        SPDR = (uint8_t)(address);
      sprintf(msg, "address %02x\r\n",(uint8_t)(address));
      printuart(msg);
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
        printuart("E\r\n");
        DESELECT_SERIAL_MEMORY;   // Pull high the chip select line of the SPI serial memory                   // terminate current write transfer
        state = READY_TO_SEND;    // return to the idle state
      } else {      
        printuart("X\r\n");
        byte_cnt ++;
        SPDR = *data_ptr;
      sprintf(msg, "data %02x\r\n",*data_ptr);
      printuart(msg);
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

uint8_t read_status_reg(uint8_t *status) {
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

/* Read status register without disablign and enabling SPI_INTERRUPTS */
uint8_t quick_read_status_reg(uint8_t *status) {
    if (state == READY_TO_SEND) {
        SELECT_SERIAL_MEMORY;        // Pull down chip select.
        spi_tx_byte(RDSR);           // Send Read status register opcode.
        *status = spi_tx_byte(0xFF); // get the status register value, by sending 0xFF we avoid toggling the MOSI line.
        DESELECT_SERIAL_MEMORY;

        return TRANSFER_COMPLETED;
    } else {
        *status = 1;
        return BUSY;
    }

}

uint8_t write_spi_command(uint8_t op_code) {
    uint8_t status_reg;

    if (read_status_reg(&status_reg) == TRANSFER_COMPLETED) {
        if(!(status_reg & (1<<WIP))) { // Check if the memory is ready to be used. WIP is set an internal write process is in progress
            DISABLE_SPI_INTERRUPT;
            SELECT_SERIAL_MEMORY;
            spi_tx_byte(op_code);
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
        if(read_status_reg(&status_reg) == TRANSFER_COMPLETED) {
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
 *
 */
uint8_t write_status_reg(uint8_t sreg) {
    uint8_t status_reg;

    if (read_status_reg(&status_reg) == TRANSFER_COMPLETED) {// Is the SPI interface being used?
      if (!(status_reg & (1<<WIP))) { // Is a write in progress internally on the memory?
          write_spi_command(WREN); // Some memories also supports EWSR
          DISABLE_SPI_INTERRUPT;
          SELECT_SERIAL_MEMORY;
          spi_tx_byte(WRSR);
          spi_tx_byte(sreg);
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

/**
 * Writes a 4 byte pattern to the memory 
 */
//uint8_t write_pattern(uint32_t pattern) {
//    uint8_t status_reg;
//    if (read_status_reg(&status_reg) == TRANSFER_COMPLETED) {
//        if (!(status_reg & (1<<WIP))) { // Check if the memory is ready to be used. WIP is set an internal write process is in progress
//            // TODO implement check of write protect
//            // some memories have three BP bits, others two
//            spi_tx
//}
uint8_t write_byte_array(uint32_t start_addr, uint8_t n_bytes, uint8_t *src) {
    uint8_t status_reg;
    if (start_addr <= TOP_ADDR) {
        if (read_status_reg(&status_reg) == TRANSFER_COMPLETED) {// Is the SPI interface being used?
            if (!(status_reg & (1<<WIP))) { // Is a write in progress internally on the memory?
                // TODO should test for write protectioin
                write_spi_command(WREN); // Write enable always has to be sent before a write operation.
                // Seeting the state to INSTRUCTION will cause the spi interrupt to send the first
                // byte of the three byte address.
                state = INSTRUCTION; 
                // global variables used by interrupt function
                nb_byte = n_bytes; 
                byte_cnt = 0;
                address = start_addr;
                data_ptr = src;
                SELECT_SERIAL_MEMORY;

                SPDR = BYTE_PRG; // write byte program command to SPI
                return TRANSFER_STARTED;
            } else {
                return BUSY;
            }
        } else {
            return BUSY;
        }
    } else {
        return OUT_OF_RANGE;
    }
}


/** 
 * Get Memory JEDEC ID 
 * 
 * Returns the JEDEC ID of the serial memory.
 */
uint8_t get_jedec_id(uint8_t *memID)
{
    uint8_t status_reg;
    if (read_status_reg(&status_reg) == TRANSFER_COMPLETED)
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

static void printuart(char *msg) {
    while (*msg != '\0') {
        USART1SendByte(*msg);
        *msg++;
    }
}

uint8_t erase_chip(void) {
    uint8_t status_reg;                                          
    if (read_status_reg(&status_reg) == TRANSFER_COMPLETED) 
    {  
        if (!(status_reg & (1<<WIP)))                  
        {
            write_spi_command(WREN);    
            DISABLE_SPI_INTERRUPT;  
            SELECT_SERIAL_MEMORY;    
            spi_tx_byte(CHIP_ERASE); 
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

/* Auto address increment word programming using software end-of-write detection */
uint8_t write_aai_soft(uint32_t start_addr, uint8_t n_bytes, uint8_t *src) {
    uint8_t status_reg;
    if (start_addr <= TOP_ADDR) {
        if (read_status_reg(&status_reg) == TRANSFER_COMPLETED) {// Is the SPI interface being used?
            if (!(status_reg & (1<<WIP))) { // Is a write in progress internally on the memory?
                write_spi_command(WREN);
                DISABLE_SPI_INTERRUPT;
                address = start_addr;
                nb_byte = n_bytes; 
                byte_cnt = 2;
                data_ptr = src;
                SELECT_SERIAL_MEMORY;
                spi_tx_byte(AAI);
                spi_tx_byte((uint8_t)(address>>16));
                spi_tx_byte((uint8_t)(address>>8));
                spi_tx_byte((uint8_t)(address));
                spi_tx_byte(*data_ptr); // send first half of word
                data_ptr++;          
                spi_tx_byte(*data_ptr); // send second half of word
                DESELECT_SERIAL_MEMORY;
                while (byte_cnt < nb_byte) {
                    // wait for internal write to finish
                    do {
                        quick_read_status_reg(&status_reg);
                    } while (status_reg & (1<<WIP));
                    SELECT_SERIAL_MEMORY;
                    spi_tx_byte(AAI);
                    data_ptr++;
                    spi_tx_byte(*data_ptr); // send first half of word
                    data_ptr++;          
                    //SPDR = *data_ptr; // Don't need to wait for transfer to finish, just wait untill internal write is done
                    spi_tx_byte(*data_ptr); // send first half of word
                    DESELECT_SERIAL_MEMORY;
                    byte_cnt += 2;
                }
                while (write_spi_command(WRDI) != TRANSFER_COMPLETED);

                ENABLE_SPI_INTERRUPT;
                return TRANSFER_COMPLETED;
            } else {
            return BUSY;
            }
        } else {
            return BUSY;
        }
    } else {
        return OUT_OF_RANGE;
    }
}

uint8_t aai_pattern() {
    uint8_t status_reg;
    uint8_t ptrn[2] = {0x55,0xAA};
    uint32_t cnt = 0;
    if (read_status_reg(&status_reg) == TRANSFER_COMPLETED) {// Is the SPI interface being used?
        if (!(status_reg & (1<<WIP))) { // Is a write in progress internally on the memory?
            write_spi_command(WREN);
            DISABLE_SPI_INTERRUPT;
            cnt = 2;
            SELECT_SERIAL_MEMORY;
            spi_tx_byte(AAI);
            spi_tx_byte(0);
            spi_tx_byte(0);
            spi_tx_byte(0);
            spi_tx_byte(ptrn[0]); // send first half of word
            spi_tx_byte(ptrn[1]); // send second half of word
            DESELECT_SERIAL_MEMORY;
            while (cnt < TOP_ADDR) {
                // wait for internal write to finish
                do {
                    quick_read_status_reg(&status_reg);
                } while (status_reg & (1<<WIP));
                SELECT_SERIAL_MEMORY;
                spi_tx_byte(AAI);
                spi_tx_byte(ptrn[0]); // send first half of word
                spi_tx_byte(ptrn[1]); // send first half of word
                DESELECT_SERIAL_MEMORY;
                cnt += 2;
            }
            while (write_spi_command(WRDI) != TRANSFER_COMPLETED);

            ENABLE_SPI_INTERRUPT;
            return TRANSFER_COMPLETED;
        } else {
        return BUSY;
        }
    } else {
        return BUSY;
    }
}

int main (void) {
    uint16_t i = 0;
    uint32_t error_cnt;
    uint8_t JEDEC_ID;
    uint8_t reg_status;
    static uint8_t dest[257*sizeof(uint8_t)];
   // static uint8_t src[10];
    // initialize the data packet indexes
    data_arr[0].index = 0;
    data_arr[1].index = 0;

   // src[0] = 0x55;
   // src[1] = 0xAA;
   // src[2] = 0x55;
   // src[3] = 0xAA;
   // src[4] = 0x55;
   // src[5] = 0xAA;
   // src[6] = 0x55;
   // src[7] = 0xAA;
   // src[8] = 0x55;
   // src[9] = 0xAA;

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

    char msg[256];
    uint32_t page_num = 0xfffff+1;
    error_cnt = 0;
    page_num = page_num/255+1;
    write_status_reg(0x00);
    //erase_chip();
    read_status_reg(&reg_status);
    sprintf(msg,"reg status: 0x%02x\r\n",reg_status);
    printuart(msg);
    //while(write_byte_array(6,1,src) != TRANSFER_STARTED);
    //while(aai_pattern() != TRANSFER_COMPLETED);
    printuart("STARTING MEMORY CHECK!\r\n");
    while(read_byte_arr(0,255,dest) != TRANSFER_COMPLETED);
    for (i = 0; i < 255; i++) {
        if ((i & 1) && (dest[i] != 0xaa)) {
            //odd
            error_cnt++;
            sprintf(msg, "Erro: addr %d should be 0xaa is %02x\r\n", i, dest[i]);
     //       put_data(&data_arr[write_i], i, 0x5, ADDR_SEU_AA);
            printuart(msg);
        } else if ( !(i & 1) && (dest[i] != 0x55)) {
            error_cnt++;
            sprintf(msg, "Erro: addr %d should be 0xaa is %02x\r\n", i, dest[i]);
      //      put_data(&data_arr[write_i], i, 0x5, ADDR_SEU_55);
            printuart(msg);
        } 
    }

    //send_packet(&data_arr[read_i]);
    //while (addr<0xfffff){
    //    read_byte_arr(addr,255,dest);
    //    addr += 255;
    //    for (i = 0; i < 255; i++) {
    //        if ((i & 1) && (dest[i] != 0xaa)) {
    //            //odd
    //            error_cnt++;
    //            sprintf(msg, "Erro: addr %d should be 0xaa is %d\r\n", i, dest[i]);
    //            printuart(msg);
    //        } else if ( !(i & 1) && (dest[i] != 0x55)) {
    //            error_cnt++;
    //            sprintf(msg, "Erro: addr %d should be 0xaa is %d\r\n", i, dest[i]);
    //            printuart(msg);
    //        } 
    //    }
    //    page_cnt++;
    //    sprintf(msg, "Checked page %lu of %lu\r\n", page_cnt, page_num);
    //    printuart(msg);
    //}
    printuart("DONE!\r\n");
    sprintf((char *)msg, "Detected %lu errors\r\n", error_cnt);
    printuart(msg);

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
