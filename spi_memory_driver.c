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
        DESELECT_SERIAL_MEMORY;      // spi_tx_byte is called a second time to wait for SPDR to be filled
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
 *
 * @addr the start address to read from
 *
 * @n_bytes the number of bytes to read
 *
 * @*dest pointer to array into which the read bytes are written
 */
uint8_t read_byte_arr(uint32_t addr, uint16_t n_bytes, uint8_t *dest) {
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
