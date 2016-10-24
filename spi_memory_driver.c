#include "memories.h"
#include "spi_memory_driver.h"

void SPI_Init(void) {
    //SPCR = (0<<SPE);  // Disable the SPI to be able to configure the #SS line as an input even if the SPI is configured as a slave
    // Set MOSI, SCK , and SS as Output
    //DDRB=(1<<MOSI)|(1<<SCK)|(1<<SS1); DDRB set as output in HAL.c
    //DESELECT_SERIAL_MEMORY;
    //DISABLE_MISO_INTERRUPT; // To avoid triggering a write stop interrupt when reading

    // Enable SPI, Set as Master
    // Prescaler: Fosc/16, Enable Interrupts
    //The MOSI, SCK pins are as per ATMega8
    //DDRB |= (1<<MOSI);
    //DDRB |= (1<<SCK);
    //DDRB &= ~(1<<MISO);
    DDRB = (1<<DDB0)|(1<<DDB1)|(1<<DDB2)|(1<<DDB4)|(1<<DDB5)|(1<<DDB6)|(1<<DDB7);//0xF7;//(1<<MOSI)|(1<<SCK);
    SPCR=(1<<SPE)|(1<<MSTR)|(1<<SPR0);//|(1<<SPIE);

    // Clear the SPIF flag by reading SPSR and SPDR
    SPSR;
    SPDR;

    state = READY_TO_SEND;
}

/*! \brief  the SPI  Transfer Complete Interrupt Handler
 *
 * The SPI interrupt handler manages the accesses to the I/O Reg SPDR during a memory write cycle.
 * The values of the global variable(state,  byte_cnt, address, data_ptr) enables the handler to compute the next byte to be send
 * over the SPI interface as well as the next state. The finite state machine diagram is provided in the application note document.  
 *
 *  \return  void.
 */
//ISR(SPI_STC_vect)
//{
//  char msg[256];
//  switch (state) {
//    ///////////////////////////////
//    // INSTRUCTION STATE
//    //////////////////////////////
//    case INSTRUCTION :
//    {
//      printuart("I\r\n");
//      state = ADDRESS;        
//      byte_cnt = 1;
//      SPDR = (uint8_t)(address>>16);      // Address phase is 3 byte long for SPI flashes
//      sprintf(msg, "address %02x\r\n",(uint8_t)(address>>16));
//      printuart(msg);
//     break;
//    }
//
//    ///////////////////////////////
//    // ADDRESS STATE
//    //////////////////////////////
//    case ADDRESS :
//    {
//      if (byte_cnt == NB_ADDR_BYTE) {    // is the last address byte reached?
//        printuart("B\r\n");
//        state = DATA;          // go to the DATA state
//        byte_cnt = 0;
//        SPDR = *data_ptr;     // send the first byte
//      sprintf(msg, "data %02x\r\n",*data_ptr);
//      printuart(msg);
//      } else if (byte_cnt == 1) {    // must the middle address byte be sent?
//        printuart("C\r\n");
//        byte_cnt ++;
//        SPDR = (uint8_t)(address>>8);
//      sprintf(msg, "address %02x\r\n",(uint8_t)(address>>8));
//      printuart(msg);
//      } else {
//        printuart("D\r\n");
//        byte_cnt ++;
//        SPDR = (uint8_t)(address);
//      sprintf(msg, "address %02x\r\n",(uint8_t)(address));
//      printuart(msg);
//      }
//      break;
//    } 
//
//    ///////////////////////////////
//    // DATA STATE
//    //////////////////////////////
//    case DATA :
//    {
//      data_ptr++;                 // point to the next byte (even if it was the last)  
//      if (byte_cnt == nb_byte ) {  // is the last byte sent?
//        printuart("E\r\n");
//        DESELECT_SERIAL_MEMORY;   // Pull high the chip select line of the SPI serial memory                   // terminate current write transfer
//        state = READY_TO_SEND;    // return to the idle state
//      } else {      
//        printuart("X\r\n");
//        byte_cnt ++;
//        SPDR = *data_ptr;
//      sprintf(msg, "data %02x\r\n",*data_ptr);
//      printuart(msg);
//      }
//      break;
//    }
//    
//    default :
//    {
//      state = READY_TO_SEND;
//    }
//  }  
//}

uint8_t spi_tx_byte(volatile uint8_t byte) {
    SPDR = byte;
    while (!(SPSR & (1<<SPIF)));
    return SPDR;
}


uint8_t read_status_reg(uint8_t *status, uint8_t mem_idx) {
  //  if (state == READY_TO_SEND) {
        CHIP_SELECT(mem_idx);
        spi_tx_byte(RDSR);           // Send Read status register opcode.
        *status = spi_tx_byte(0xFF); // get the status register value, by sending 0xFF we avoid toggling the MOSI line.
        CHIP_DESELECT(mem_idx);

        return TRANSFER_COMPLETED;
  // } else {
  //      *status = 1;
  //      return BUSY;
  //  }

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

uint8_t spi_command(uint8_t op_code, uint8_t mem_idx)
{
    CHIP_SELECT(mem_idx);
    spi_tx_byte(op_code);
    CHIP_DESELECT(mem_idx);
    return 0;
}

/*
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
*/

uint8_t read_24bit_page(uint32_t addr, uint8_t mem_idx, uint8_t *buffer) 
{
    uint16_t read_cnt=0;     // WARNING! Should be the same type as page size, to avoid integer overflow when reading
    //uint8_t *buffer_ptr;    // temporary variable for incrementing buffer pointer
    //buffer_ptr = buffer;
    uint16_t page_size = mem_arr[mem_idx].page_size;
    uint8_t status_reg;

    read_status_reg(&status_reg, mem_idx);
    if (!(status_reg & (1<<WIP))) { // Is internal write or erase is currently in progress?

        CHIP_SELECT(mem_idx);
        spi_tx_byte(READ);
        spi_tx_byte((uint8_t)(addr>>16)); // Send the MSB byte. Casting to uint8_t will send onlyt the lower byte
        spi_tx_byte((uint8_t)(addr>>8));  // Send the middle byte
        spi_tx_byte((uint8_t)(addr));     // Send the LSB byte
        while (read_cnt < page_size) {
            //*buffer_ptr++ = spi_tx_byte(0xFF);
            buffer[read_cnt] = spi_tx_byte(0xFF);
            //USART0SendByte(buffer[read_cnt]);
            read_cnt++;
        }
        CHIP_DESELECT(mem_idx);
        return TRANSFER_COMPLETED;
    } else {
        return BUSY;
    }
}

/**
 * Reads one or more bytes from the SPI Memory
 *
 * @addr the start address to read from
 *
 * @n_bytes the number of bytes to read
 *
 * @*dest pointer to array into which the read bytes are written
uint8_t read_byte_arr(uint32_t addr, uint16_t n_bytes, uint8_t *dest) 
{
    uint8_t status_reg;
    uint8_t access_status;
    uint16_t read_cnt;     // WARNING! Should be the same type as n_bytes, to avoid integer overflow when reading
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

                //enable_pin_macro(*(mem.cs_port), mem.PIN_CS);
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
 */

/**
 *
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
 */


/** 
 * Write a page to memory size of the page is contained in the mem_arr[i] struct
 * @addr first address of page
 * @pattern each page should have a different pattern, so that a SEFI that adresses the wrong page can
 * be detected. 0 for 0x55 0xAA, 1 for 0xAA 0x55.
 * @mem_idx which of the 12 memories to write to, indexed 0 through 11
 */
uint8_t write_24bit_page(uint32_t addr, uint8_t ptr_i, uint8_t mem_idx)
{
    uint16_t write_cnt = 0;
    uint16_t page_size = mem_arr[mem_idx].page_size;
    uint8_t pattern[2] = {0x55,0xAA};
    uint8_t i = ptr_i;
    uint8_t status_reg;

    //if (start_addr <= TOP_ADDR) {
    read_status_reg(&status_reg, mem_idx);
    if (!(status_reg & (1<<WIP))) { // Is internal write or erase is currently in progress?
        // TODO should test for write protectioin

        spi_command(WREN, mem_idx); // Write enable always has to be sent before a write operation.
        CHIP_SELECT(mem_idx);
        spi_tx_byte(PRG);
        spi_tx_byte((uint8_t)(addr>>16)); // Send the MSB byte. Casting to uint8_t will send onlyt the lower byte
        spi_tx_byte((uint8_t)(addr>>8));  // Send the middle byte
        spi_tx_byte((uint8_t)(addr));     // Send the LSB byte
        while (write_cnt < page_size) {
            spi_tx_byte(pattern[i]);
            i ^= 0x01;
            write_cnt++;
        }
        CHIP_DESELECT(mem_idx);
        return TRANSFER_COMPLETED;
    } else {
        return BUSY;
    }
}
/**
 * Writes a 4 byte pattern to the memory 
 * This function is for Microchip memories SST
 * which use interrupt driven writing

uint8_t write_byte_array(uint32_t start_addr, uint8_t n_bytes, uint8_t *src)
{
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

                SPDR = PRG; // write byte program command to SPI
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
 */

/**
 * Enables the VCC of the memory whose information is stored in the Memory struct mem.
 * This is done by pulling the input pin of the inverter low, which will pull up the correspoding pin
 * on the output of the inverter which is connected to the corresponding memory.
 */
void enable_memory_vcc(struct Memory mem)
{
    disable_pin_macro(*(mem.vcc_port), mem.PIN_VCC);
}

/**
 * Disables the VCC of the memory whose information is stored in the Memory struct mem.
 * This is done by pulling the input pin of the inverter low, which will pull up the correspoding pin
 * on the output of the inverter which is connected to the corresponding memory.
 * TODO Consider using just a macro for less overhead
 */
void disable_memory_vcc(struct Memory mem)
{
    enable_pin_macro(*(mem.vcc_port), mem.PIN_VCC);
}

/** 
 * Get Memory JEDEC ID 
 * 
 * Returns the JEDEC ID of the serial memory.
 */
uint8_t get_jedec_id(uint8_t mem_idx, uint8_t* memID)
{
    uint8_t status_reg;
    read_status_reg(&status_reg, mem_idx);
    if (!(status_reg & (1<<WIP))) {// Check if the memory is ready to be used. WIP is set an internal write process is in progress
        //DISABLE_SPI_INTERRUPT;
        //SELECT_SERIAL_MEMORY;
        CHIP_SELECT(mem_idx);
        spi_tx_byte(JEDEC);    // Transmit JEDEC-ID opcode
        spi_tx_byte(0xff);     // Recieve manufacturer ID
        *memID = spi_tx_byte(0xff);     // Memory ID
        //DESELECT_SERIAL_MEMORY;
        // ENABLE_SPI_INTERRUPT;
        CHIP_DESELECT(mem_idx);
        //
    } else {
        return BUSY;
    }
}

uint8_t erase_chip(uint8_t mem_idx)
{
    uint8_t status_reg;                                          
    read_status_reg(&status_reg, mem_idx);
    if (!(status_reg & (1<<WIP))) {
            spi_command(WREN, mem_idx);    
    //        DISABLE_SPI_INTERRUPT;  
            CHIP_SELECT(mem_idx);
            spi_tx_byte(CHIP_ERASE); 
            CHIP_DESELECT(mem_idx);
            return TRANSFER_COMPLETED;
    } else {
        return BUSY;
    }
}

/*
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
*/

/* Auto address increment word programming using software end-of-write detection */
/*
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
*/
